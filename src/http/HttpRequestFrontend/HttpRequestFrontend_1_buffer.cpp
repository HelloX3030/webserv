#include "http/HttpRequestFrontend.hpp"
#include "HttpRequestFrontend_internal.hpp"

/* scan buffer_ for CRLF ("\r\n").
on success: pos set to index of '\r', returns true.
on failure: pos unchanged, returns false.

CRLF is the HTTP/1.1 line terminator (RFC 9112 section 2.2).
bare LF is non-conformant; this function requires the full sequence. */
bool HttpRequestFrontend::find_crlf(size_t& pos) const
{
    if (buffer_.size() < CRLF_LEN)
        return false;

    size_t limit = buffer_.size() - 1;
    for (size_t i = 0; i < limit; ++i)
    {
        if (buffer_[i] == '\r' && buffer_[i + 1] == '\n')
        {
            pos = i;
            return true;
        }
    }
    return false;
}

/* return line content: buffer_[0..crlf_pos), excluding CRLF.
precondition: find_crlf() returned true with this crlf_pos. */
std::string_view HttpRequestFrontend::extract_line(size_t crlf_pos) const
{
    return std::string_view(buffer_.data(), crlf_pos);
}

/* erase line including its CRLF terminator: buffer_[0..crlf_pos + CRLF_LEN).
called after successfully parsing a line.
what remains in buffer_ is unparsed input for subsequent phases. */
void HttpRequestFrontend::consume_line(size_t crlf_pos)
{
    buffer_.erase(0, crlf_pos + CRLF_LEN);
}
