#pragma once

#include <cstddef>

/* CRLF: the HTTP/1.1 line terminator (RFC 9112 section 2.2).
carriage return ('\r', 0x0D) followed by line feed ('\n', 0x0A). */
constexpr std::size_t CRLF_LEN = 2;  // sizeof '\r' + sizeof '\n'

// ctrl flow between phase parsers
enum class PhaseResult
{
    Advanced, // phase completed, transitioned to next
    NeedMore, // insufficient bytes, remain in phase
    Failed    // parse error, transitioned to ERROR
};

/* sub-state within BODY phase when chunked encoding is active.
alternates between reading a chunk-size line and reading chunk data. */
enum class ChunkPhase
{
    SIZE, // waiting for hex chunk-size CRLF
    DATA  // consuming chunk_remaining_ bytes, then CRLF
};
