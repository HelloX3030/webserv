#include "http/HttpRequestFrontend.hpp"

/* scan buffer_ for CRLF ("\r\n").
on success: pos set to index of '\r', returns true.
on failure: pos unchanged, returns false.

CRLF is the HTTP/1.1 line terminator (RFC 9112 section 2.2).
bare LF is non-conformant; we require the full sequence. */
bool HttpRequestFrontend::find_crlf(size_t& pos) const
{
    if (buffer_.size() < 2)
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

/* extract line content from buffer_[0] to buffer_[crlf_pos].
excludes the CRLF itself.
precondition: crlf_pos < buffer_.size() - 1,
              buffer_[crlf_pos] == '\r', buffer_[crlf_pos+1] == '\n'. */
std::string_view HttpRequestFrontend::extract_line(size_t crlf_pos) const
{
    return std::string_view(buffer_.data(), crlf_pos);
}

/* erase buffer_[0..pos] inclusive.
used after successfully parsing a line or consuming body bytes.
what remains in buffer_ is unparsed input for subsequent phases. */
void HttpRequestFrontend::consume_through(size_t pos)
{
    buffer_.erase(0, pos + 1);
}
