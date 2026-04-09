#include "HttpRequestFrontend_internal.hpp"
#include "http/HttpRequestFrontend.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <stdexcept>

/* trim leading and trailing OWS (SP / HTAB) from a string_view.
RFC 9110 section 5.6.3: field-value is bounded by optional whitespace
on either side of the colon — that whitespace is not part of the value. */
static std::string_view trim_ows(std::string_view s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string_view::npos)
        return {};
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

/* header phase parser. called once per advance() loop iteration
while phase_ == HEADERS.

2 outcomes per call:
    empty line (crlf_pos == 0): headers complete.
        enforce Host (HTTP/1.1), determine body encoding,
        set body_remaining_ or body_chunked_, transition phase_.
    non-empty line: one header parsed and accumulated.
        remain in HEADERS; advance() loops back. */
PhaseResult HttpRequestFrontend::parse_header_line()
{
    assert(phase_ == ParsePhase::HEADERS);

    size_t crlf_pos;
    if (!find_crlf(crlf_pos))
        return PhaseResult::NeedMore;

    /* check cumulative header size before processing.
    line length = crlf_pos, plus CRLF terminator.
    RFC 6585 §5: 431 if header section exceeds limit. */
    size_t line_bytes = crlf_pos + CRLF_LEN;
    if (header_bytes_ + line_bytes > MAX_HEADER_BYTES)
    {
        error_code_ = 431;
        phase_ = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }
    header_bytes_ += line_bytes;

    std::string_view line = extract_line(crlf_pos);

    /* empty line: the blank CRLF terminating the header section. */
    if (line.empty())
    {
        /* RFC 9112 §3.2: HTTP/1.1 requests without a Host header
        must be rejected with 400. checked here — the earliest point
        at which the complete header set is available. */
        if (request_.http_version == "HTTP/1.1" &&
            request_.headers.find("host") == request_.headers.end())
        {
            error_code_ = 400;
            phase_ = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

        auto te = request_.headers.find("transfer-encoding");
        bool chunked = false;
        if (te != request_.headers.end())
        {
            std::string encoding = te->second;
            std::transform(encoding.begin(), encoding.end(), encoding.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            chunked = (encoding == "chunked");
        }

        if (chunked)
        {
            /* subject (§7 CGI): server un-chunks before passing body
            to CGI; CGI receives EOF-terminated plain stream.
            413 cannot be checked here — decoded size is unknown
            until chunks are accumulated. checked per-chunk in BODY. */
            body_chunked_ = true;
            chunk_remaining_ = 0;
            chunk_phase_ = ChunkPhase::SIZE;
            body_remaining_ = 0;
            consume_line(crlf_pos);
            phase_ = ParsePhase::BODY;

            assert(phase_ == ParsePhase::BODY);
            assert(body_chunked_ == true);
            assert(chunk_phase_ == ChunkPhase::SIZE);

            return PhaseResult::Advanced;
        }

        /* Content-Length path. absent → 0 (no body). */
        size_t content_length = 0;
        auto cl = request_.headers.find("content-length");
        if (cl != request_.headers.end())
        {
            try
            {
                size_t pos;
                long n = std::stol(cl->second, &pos);
                /* pos must reach end of string: no trailing garbage.
                stol stops at the first non-digit; if pos < size(),
                the value is not a pure integer (e.g. "42abc"). */
                if (pos != cl->second.size() || n < 0)
                {
                    error_code_ = 400;
                    phase_ = ParsePhase::ERROR;
                    return PhaseResult::Failed;
                }
                content_length = static_cast<size_t>(n);
            }
            catch (const std::exception &)
            {
                error_code_ = 400;
                phase_ = ParsePhase::ERROR;
                return PhaseResult::Failed;
            }
        }

        if (content_length > max_body_size_)
        {
            error_code_ = 413;
            phase_ = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

        body_chunked_ = false;
        body_remaining_ = content_length;
        consume_line(crlf_pos);
        phase_ = (body_remaining_ == 0) ? ParsePhase::COMPLETE
                                        : ParsePhase::BODY;

        assert(body_remaining_ <= max_body_size_);
        assert(body_chunked_ == false);
        assert(phase_ == ParsePhase::BODY || phase_ == ParsePhase::COMPLETE);

        return PhaseResult::Advanced;
    }

    /* non-empty line: parse one header field.
    field-line = field-name ":" OWS field-value OWS */
    size_t colon = line.find(':');
    if (colon == std::string_view::npos || colon == 0)
    {
        error_code_ = 400;
        phase_ = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    std::string_view raw_name = line.substr(0, colon);
    std::string_view raw_value = line.substr(colon + 1);

    /* normalise name to lowercase: quotient over the case-equivalence
    relation on field names (RFC 9110 §5.1). all consumers downstream
    operate on the quotient space, never the raw surface form. */
    std::string name(raw_name);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    std::string value(trim_ows(raw_value));

    /* Content-Length: duplicate with differing values is a smuggling vector.
    RFC 9110 §8.6: differing values must be rejected. same value may be
    collapsed to 1 instance. we reject all duplicates for simplicity. */
    if (name == "content-length")
    {
        auto it = request_.headers.find("content-length");
        if (it != request_.headers.end())
        {
            if (it->second != value)
            {
                error_code_ = 400;
                phase_ = ParsePhase::ERROR;
                return PhaseResult::Failed;
            }
            /* same value — keep first, ignore duplicate */
            consume_line(crlf_pos);
            return PhaseResult::Advanced;
        }
    }

    /* general headers: comma-concat per RFC 9110 §5.3.
    multiple field lines with the same name are combined into
    1 field value, separated by ", ". */
    auto it = request_.headers.find(name);
    if (it == request_.headers.end())
        request_.headers[name] = std::move(value);
    else
        it->second += ", " + value;

    consume_line(crlf_pos);
    return PhaseResult::Advanced;
}
