#include "http/HttpRequestFrontend.hpp"
#include "HttpRequestFrontend_internal.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

/* parse a hex chunk-size string into size_t.
RFC 9112 §7.1: chunk-size = 1*HEXDIG.
returns false on: empty input, non-hex characters, overflow.

pure function — no state, no I/O. */
static bool parse_hex(std::string_view s, size_t& out)
{
    if (s.empty())
        return false;
    try
    {
        size_t        pos;
        unsigned long val = std::stoul(std::string(s), &pos, 16);
        if (pos != s.size())
            return false;
        out = static_cast<size_t>(val);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

/* body phase parser. branches on body_chunked_.

Content-Length path:
    consume exactly body_remaining_ bytes. trivial accumulation.

chunked path (RFC 9112 §7.1):
    sub-state machine alternating SIZE and DATA.

    SIZE: scan for CRLF, parse hex chunk-size.
        chunk_size == 0 → last-chunk: consume size line + trailer
        CRLF atomically, transition → COMPLETE.
        chunk_size > 0 → check 413, consume size line,
        set chunk_remaining_, transition → DATA.

    DATA: wait for chunk_remaining_ + CRLF_LEN bytes.
        validate trailing CRLF. append decoded bytes to
        request_.body. consume from buffer_. transition → SIZE.

    returns Advanced after each sub-state transition.
    the while loop in advance() re-enters consume_body(),
    processing all available chunks within a single advance() call.

error assignment:
    invalid hex in chunk-size line              → 400
    missing or malformed CRLF after chunk data  → 400
    decoded body exceeds max_body_size_         → 413 */
PhaseResult HttpRequestFrontend::consume_body()
{
    assert(phase_ == ParsePhase::BODY);

    /* ── Content-Length path ── */

    if (!body_chunked_)
    {
        assert(body_remaining_ <= max_body_size_);

        if (buffer_.size() < body_remaining_)
            return PhaseResult::NeedMore;

        /* invariant: buffer_ may contain more than body_remaining_
        (pipelined requests). extract exactly body_remaining_,
        leave the rest for the next request after reset(). */
        request_.body.append(buffer_, 0, body_remaining_);
        buffer_.erase(0, body_remaining_);
        phase_ = ParsePhase::COMPLETE;

        assert(phase_ == ParsePhase::COMPLETE);

        return PhaseResult::Advanced;
    }

    /* ── chunked path: sub-state machine (SIZE / DATA) ── */

    if (chunk_phase_ == ChunkPhase::SIZE)
    {
        size_t crlf_pos;
        if (!find_crlf(crlf_pos))
            return PhaseResult::NeedMore;

        std::string_view line = extract_line(crlf_pos);

        /* strip chunk extensions (RFC 9112 §7.1.1):
        chunk = chunk-size [ chunk-ext ] CRLF chunk-data CRLF
        chunk-ext = *( BWS ";" BWS chunk-ext-name [ ... ] )
        we ignore extensions: truncate at semicolon. */
        size_t semi = line.find(';');
        if (semi != std::string_view::npos)
            line = line.substr(0, semi);

        size_t chunk_size;
        if (!parse_hex(line, chunk_size))
        {
            error_code_ = 400;
            phase_      = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

        if (chunk_size == 0)
        {
            /* last-chunk. consume size line CRLF + trailer CRLF
            atomically — no third sub-phase needed.
            buffer must contain both before we proceed. */
            size_t total = crlf_pos + CRLF_LEN + CRLF_LEN;
            if (buffer_.size() < total)
                return PhaseResult::NeedMore;

            /* validate trailer terminator */
            size_t trailer_start = crlf_pos + CRLF_LEN;
            if (buffer_[trailer_start]     != '\r' ||
                buffer_[trailer_start + 1] != '\n')
            {
                error_code_ = 400;
                phase_      = ParsePhase::ERROR;
                return PhaseResult::Failed;
            }

            buffer_.erase(0, total);
            phase_ = ParsePhase::COMPLETE;

            assert(phase_ == ParsePhase::COMPLETE);
            assert(request_.body.size() <= max_body_size_);

            return PhaseResult::Advanced;
        }

        /* non-zero chunk: check 413 against decoded accumulation.
        earliest point at which we know the decoded size will
        exceed the limit — before allocating or copying. */
        if (request_.body.size() + chunk_size > max_body_size_)
        {
            error_code_ = 413;
            phase_      = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

        consume_line(crlf_pos);
        chunk_remaining_ = chunk_size;
        chunk_phase_     = ChunkPhase::DATA;

        assert(chunk_phase_ == ChunkPhase::DATA);
        assert(chunk_remaining_ > 0);

        return PhaseResult::Advanced;
    }

    /* chunk_phase_ == ChunkPhase::DATA
    need chunk_remaining_ data bytes + trailing CRLF. */

    assert(chunk_phase_ == ChunkPhase::DATA);
    assert(chunk_remaining_ > 0);

    size_t need = chunk_remaining_ + CRLF_LEN;
    if (buffer_.size() < need)
        return PhaseResult::NeedMore;

    /* validate trailing CRLF after chunk data.
    if absent, the byte stream is corrupt — no recovery. */
    if (buffer_[chunk_remaining_]     != '\r' ||
        buffer_[chunk_remaining_ + 1] != '\n')
    {
        error_code_ = 400;
        phase_      = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    request_.body.append(buffer_, 0, chunk_remaining_);
    buffer_.erase(0, need);
    chunk_phase_ = ChunkPhase::SIZE;

    assert(chunk_phase_ == ChunkPhase::SIZE);
    assert(request_.body.size() <= max_body_size_);

    return PhaseResult::Advanced;
}
