#pragma once

#include "http/HttpRequestFrontend.hpp"

#include <ostream>
#include <string>

/* operator<< for ParseStatus.
required by assert_eq: on failure, the template instantiation
calls os << expected and os << actual. without this overload,
the compiler cannot resolve the insertion for ParseStatus.

defined as a free function in the test harness — not in production
headers. production code has no reason to serialise ParseStatus. */
inline std::ostream& operator<<(std::ostream& os, ParseStatus s)
{
    switch (s)
    {
        case ParseStatus::Incomplete: return os << "Incomplete";
        case ParseStatus::Complete:   return os << "Complete";
        case ParseStatus::Failed:     return os << "Failed";
    }
    return os << "Unknown(" << static_cast<int>(s) << ")";
}

/* feed a complete string to advance() in 1 call.
the common case: input is known to contain a full request
(or a full malformed input). */
inline ParseResult advance_all(HttpRequestFrontend& fe,
                                const std::string& input)
{
    return fe.advance(input.data(), input.size());
}

/* feed a string byte-by-byte.
exercises every possible split boundary. returns the terminal
ParseResult — the first that is not Incomplete.

if the entire input is consumed and the final result is still
Incomplete, that Incomplete is returned. this is correct:
the input was insufficient for a terminal state. */
inline ParseResult advance_byte_by_byte(HttpRequestFrontend& fe,
                                         const std::string& input)
{
    ParseResult result{};
    for (size_t i = 0; i < input.size(); ++i)
    {
        result = fe.advance(&input[i], 1);
        if (result.status != ParseStatus::Incomplete)
            return result;
    }
    return result;
}
