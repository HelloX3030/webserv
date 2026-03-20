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
        extract Content-Length, set body_remaining_, transition phase_.
    non-empty line: one header parsed and accumulated.
        remain in HEADERS; advance() loops back.

the empty-line check is a natural consequence of find_crlf semantics:
if buffer_ begins with CRLF, crlf_pos == 0 and extract_line returns
an empty string_view. no special sentinel needed. */
PhaseResult HttpRequestFrontend::parse_header_line()
{
    size_t crlf_pos;
    if (!find_crlf(crlf_pos))
        return PhaseResult::NeedMore;

    std::string_view line = extract_line(crlf_pos);

    /* empty line: the blank CRLF terminating the header section.
    all headers now in request_.headers. compute body. */
    if (line.empty())
    {
        /* Transfer-Encoding: chunked — not implemented.
        detected here rather than during header accumulation because
        the transition is the earliest point at which the complete
        set of headers is available for cross-header checks. */
        auto te = request_.headers.find("transfer-encoding");
        if (te != request_.headers.end() && te->second == "chunked")
        {
            error_code_ = 501;
            phase_      = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

        /* extract Content-Length. absent → 0 (no body).
        value was already trimmed at insertion; parse directly. */
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

        /* 413 check before allocation: reject oversized bodies
        at the earliest possible moment. */
        if (content_length > max_body_size_)
        {
            error_code_ = 413;
            phase_      = ParsePhase::ERROR;
            return PhaseResult::Failed;
        }

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

    /* normalise name to lowercase.
    RFC 9110 section 5.1: field names are case-insensitive.
    normalising at parse time means all consumers use a single form. */
    std::string name(raw_name);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    std::string value(trim_ows(raw_value));

    request_.headers[name] = std::move(value);

    consume_line(crlf_pos);
    return PhaseResult::Advanced;
}
