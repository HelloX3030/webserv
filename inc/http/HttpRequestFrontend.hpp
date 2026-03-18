#pragma once

#include "HttpRequest.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

// external result status — returned to Connection
enum class ParseStatus
{
    Incomplete, // need more bytes
    Complete,   // request fully parsed
    Failed      // parse error, error_code set
};

// internal phase result — returned by phase parsers
enum class PhaseResult
{
    Advanced, // phase completed, transitioned to next
    NeedMore, // insufficient bytes, remain in phase
    Failed    // parse error, transitioned to ERROR
};

// current parse phase
enum class ParsePhase
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

struct ParseResult
{
    ParseStatus status;
    HttpRequest request;    // valid iff Complete
    uint16_t    error_code; // valid iff Failed (400, 413, 501, 505)
};

struct HttpRequestFrontend
{
public:

        // construction
        explicit HttpRequestFrontend(size_t max_body_size);

        /* advance: append bytes to internal buffer, advance parse state.
        return as soon as status is determinable. */
        ParseResult advance(const char* data, size_t len);
        /* reset: clear state for next request on persistent connection.
        called by Connection after response sent, iff `keepAlive()` is true.
        buffer is not cleared — may contain bytes from pipelined next request. */
        void        reset();

private:
    // internal: implementation detail

    // state
    std::string buffer_;
    ParsePhase  phase_;
    HttpRequest request_;
    size_t      body_remaining_;
    uint16_t    error_code_;
    size_t      max_body_size_;

    // phase parsers
    PhaseResult parse_request_line();
    PhaseResult parse_header_line();
    PhaseResult consume_body();

    // helpers
    bool             find_crlf(size_t& pos) const;
    std::string_view extract_line(size_t crlf_pos) const;
    void             consume_through(size_t pos);
};
