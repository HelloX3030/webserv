#include "http/HttpRequestFrontend.hpp"
#include "HttpRequestFrontend_internal.hpp"

#include <cassert>
#include <cctype>

/* request-line phase parser.
RFC 9112 section 3: request-line = method SP request-target SP HTTP-version CRLF

the line is a flat byte sequence. structure is recovered by locating
exactly 2 SP separators, yielding 3 tokens. no token may be empty;
no SP may be doubled or leading/trailing (strict, per RFC 9112 section 3).

error assignment:
    token count != 3, empty token, extra SP  → 400
    non-empty unknown method                 → 501
    uri not beginning with '/'              → 400
    version not matching HTTP/D.D           → 400
    version HTTP/D.D but not 1.0 or 1.1    → 505

on success:
request_.method, request_.uri, request_.http_version populated;
buffer_ advanced past line;
phase_ → HEADERS. */
PhaseResult HttpRequestFrontend::parse_request_line()
{
    assert(phase_ == ParsePhase::REQUEST_LINE);

    size_t crlf_pos;
    if (!find_crlf(crlf_pos))
        return PhaseResult::NeedMore;

    std::string_view line = extract_line(crlf_pos);

    /* locate SP₁: separates method from uri */
    size_t sp1 = line.find(' ');
    if (sp1 == std::string_view::npos || sp1 == 0)
    {
        error_code_ = 400;
        phase_      = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    std::string_view method = line.substr(0, sp1);
    std::string_view tail   = line.substr(sp1 + 1);

    /* locate SP₂: separates uri from version */
    size_t sp2 = tail.find(' ');
    if (sp2 == std::string_view::npos || sp2 == 0)
    {
        error_code_ = 400;
        phase_      = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    std::string_view uri     = tail.substr(0, sp2);
    std::string_view version = tail.substr(sp2 + 1);

    /* version must contain no further SP — no 4th token, no trailing SP */
    if (version.empty() || version.find(' ') != std::string_view::npos)
    {
        error_code_ = 400;
        phase_      = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    /* validate method.
    methods are case-sensitive (RFC 9110 section 9.1).
    GET/POST/DELETE accepted; any other non-empty token → 501. */
    if (method != "GET" && method != "POST" && method != "DELETE")
    {
        error_code_ = 501;
        phase_      = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    /* validate uri: origin-form requires absolute-path, beginning with '/'.
    uri is non-empty here (sp2 != 0 checked above). */
    if (uri[0] != '/')
    {
        error_code_ = 400;
        phase_      = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    /* validate version.
    accepted: HTTP/1.0, HTTP/1.1.
    HTTP/D.D where D is a digit but not HTTP/1.x → 505.
    anything else → 400. */
    if (version != "HTTP/1.0" && version != "HTTP/1.1")
    {
        if (version.size() == 8          &&
            version[0] == 'H'            &&
            version[1] == 'T'            &&
            version[2] == 'T'            &&
            version[3] == 'P'            &&
            version[4] == '/'            &&
            std::isdigit((unsigned char)version[5]) &&
            version[6] == '.'            &&
            std::isdigit((unsigned char)version[7]))
        {
            error_code_ = 505;
        }
        else
        {
            error_code_ = 400;
        }
        phase_ = ParsePhase::ERROR;
        return PhaseResult::Failed;
    }

    request_.method       = std::string(method);
    request_.uri          = std::string(uri);
    request_.http_version = std::string(version);

    consume_line(crlf_pos);
    phase_ = ParsePhase::HEADERS;

    assert(phase_ == ParsePhase::HEADERS);
    assert(!request_.method.empty());
    assert(!request_.uri.empty());
    assert(!request_.http_version.empty());

    return PhaseResult::Advanced;
}
