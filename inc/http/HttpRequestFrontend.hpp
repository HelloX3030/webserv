#pragma once

#include "HttpRequest.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

// --- PUBLIC INTERFACE for Connection ---

// result status returned to Connection after each advance() call
enum class ParseStatus
{
    Incomplete, // need more bytes
    Complete,   // request fully parsed
    Failed      // parse error, error_code set
};

// result bundle returned by advance()
struct ParseResult
{
    ParseStatus status;
    HttpRequest request;    // valid iff Complete
    uint16_t    error_code; // valid iff Failed (400, 413, 501, 505)
};

// --- INTERNAL — implementation detail, do not depend on ---

// parse position
enum class ParsePhase
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    COMPLETE,
    ERROR
};

struct HttpRequestFrontend
{
    // --- PUBLIC INTERFACE for Connection ---
    // C++ structs default to public visibility,
    // classes default to private

    // construction
    explicit HttpRequestFrontend(size_t max_body_size);

    /* append bytes to internal buffer, advance parse state.
    return as soon as status is determinable. */
    ParseResult advance(const char* data, size_t len);

    /* clear state for next request on persistent connection.
    called by Connection after response sent, iff `keepAlive()` is true.
    buffer is not cleared — may contain bytes from pipelined next request. */
    void        reset();


private: // --- INTERNAL: implementation detail

    // -- state --
    std::string buffer_;            // accumulated unparsed bytes
    // consumed bytes erased after each successful phase transition.

    ParsePhase  phase_;             // current phase/parse position
    // advances monotonically (except `reset()`)

    HttpRequest request_;           // built incrementally
    size_t      body_remaining_;    // bytes still expected
    uint16_t    error_code_;        // set on ERROR transition
    size_t      max_body_size_;     // from config, for 413 detection


    // -- phase parsers --
    PhaseResult parse_request_line();
    PhaseResult parse_header_line();
    PhaseResult consume_body();


    // -- helpers --
    bool             find_crlf(size_t& pos) const;
    std::string_view extract_line(size_t crlf_pos) const;
    void             consume_line(size_t pos);
};
