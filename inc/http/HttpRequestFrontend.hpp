#pragma once

#include "HttpRequest.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

// forward declarations — definitions in HttpRequestFrontend_internal.hpp
enum class PhaseResult : int;
enum class ChunkPhase : int;

// --- PUBLIC INTERFACE for Connection ---

enum class ParseStatus : int;  // forward declaration — size known

// result product type / aggregrate (C++) returned by advance()
struct ParseResult
{
    ParseStatus status;     // determines control flow. switch(result.status) branches to: continue polling, dispatch, or error handling.
    HttpRequest request;    // payload. valid iff status == Complete. Connection passes this to routing/handling logic.
    uint16_t    error_code; // HTTP status code for error response. valid iff status == Failed. Connection uses this to construct the error response.
};

/* result status returned to caller (Connection) after each advance() call.
reflects caller's decision space:
what distinct action categories exist after calling advance()? */
enum class ParseStatus : int
{
    Incomplete, // no result yet, need more bytes. action: wait for more bytes, call advance() again.
    Complete,   // request fully parsed, valid result produced. action: dispatch request to handler.
    Failed      // unrecoverable parse error. action: respond with error, close or reset. frontend internal: error_code set
};
// mutually exclusive & jointly exhaustive

// --- INTERNAL (implementation detail): do not depend on ---

struct HttpRequestFrontend
{
    // --- nested types first ---
    enum class ParsePhase : int
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        COMPLETE,
        ERROR
    };

    // --- PUBLIC INTERFACE for Connection ---
    // construction
    explicit HttpRequestFrontend(size_t max_body_size); // sole constructor parameter (inject external dependency: configuration, not state)

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
    ParsePhase  phase_;             // current phase/parse position
    // advances monotonically (except `reset()`)
    HttpRequest request_;           // built incrementally
    size_t      body_remaining_;    // bytes still expected
    size_t      header_bytes_;      // cumulative bytes consumed in header section
    uint16_t    error_code_;        // set on ERROR transition
    size_t      max_body_size_;     // from config, for 413 detection
    bool        body_chunked_;      // Transfer-Encoding: chunked present
    size_t      chunk_remaining_;   // bytes left in current chunk (DATA sub-phase)
    ChunkPhase  chunk_phase_;       // sub-state within chunked BODY

    // -- phase parsers --
    PhaseResult parse_request_line();
    PhaseResult parse_header_line();
    PhaseResult consume_body();

    // -- helpers --
    bool             find_crlf(size_t& pos) const;
    std::string_view extract_line(size_t crlf_pos) const;
    void             consume_line(size_t pos);
};
