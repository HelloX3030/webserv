#include "http/HttpRequestFrontend.hpp"
#include "HttpRequestFrontend_internal.hpp"

#include <algorithm>
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
    size_t crlf_pos;
    if (!find_crlf(crlf_pos))
        return PhaseResult::NeedMore;

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
            phase_      = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

        auto te      = request_.headers.find("transfer-encoding");
        bool chunked = (te != request_.headers.end()
                        && te->second == "chunked");

        if (chunked)
        {
            /* subject (§7 CGI): server un-chunks before passing body
            to CGI; CGI receives EOF-terminated plain stream.
            413 cannot be checked here — decoded size is unknown
            until chunks are accumulated. checked per-chunk in BODY. */
            body_chunked_    = true;
            chunk_remaining_ = 0;
            chunk_phase_     = ChunkPhase::SIZE;
            body_remaining_  = 0;
            consume_line(crlf_pos);
            phase_ = ParsePhase::BODY;
            return PhaseResult::Advanced;
        }

        /* Content-Length path. absent → 0 (no body). */
        size_t content_length = 0;
        auto   cl = request_.headers.find("content-length");
        if (cl != request_.headers.end())
        {
            try
            {
                size_t pos;
                long   n = std::stol(cl->second, &pos);
                /* pos must reach end of string: no trailing garbage.
                stol stops at the first non-digit; if pos < size(),
                the value is not a pure integer (e.g. "42abc"). */
                if (pos != cl->second.size() || n < 0)
                {
                    error_code_ = 400;
                    phase_      = ParsePhase::ERROR;
                    return PhaseResult::Failed;
                }
                content_length = static_cast<size_t>(n);
            }
            catch (const std::exception&)
            {
                error_code_ = 400;
                phase_      = ParsePhase::ERROR;
                return PhaseResult::Failed;
            }
        }

        if (content_length > max_body_size_)
        {
            error_code_ = 413;
            phase_      = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

        body_chunked_   = false;
        body_remaining_ = content_length;
        consume_line(crlf_pos);
        phase_ = (body_remaining_ == 0) ? ParsePhase::COMPLETE
                                        : ParsePhase::BODY;
        return PhaseResult::Advanced;
    }

    /* non-empty line: parse one header field.
    field-line = field-name ":" OWS field-value OWS */
    size_t colon = line.find(':');
    if (colon == std::string_view::npos || colon == 0)
    {
        error_code_ = 400;
        phase_      = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    std::string_view raw_name  = line.substr(0, colon);
    std::string_view raw_value = line.substr(colon + 1);

    /* normalise name to lowercase: quotient over the case-equivalence
    relation on field names (RFC 9110 §5.1). all consumers downstream
    operate on the quotient space, never the raw surface form. */
    std::string name(raw_name);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string value(trim_ows(raw_value));

    /* duplicate headers: last value wins.
    RFC 9112 §6.3.5 permits rejecting duplicate Content-Length
    with differing values as 400; subject is silent on this.
    last-value-wins is acceptable for webserv's scope. */
    request_.headers[name] = std::move(value);

    consume_line(crlf_pos);
    return PhaseResult::Advanced;
}
