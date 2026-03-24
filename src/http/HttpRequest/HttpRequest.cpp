#include "http/HttpRequest.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

/* content-length header value as a signed integer.
headers are normalised to lowercase at parse time,
so the key is "content-length".
the value must be a non-negative decimal integer;
anything else (absent, non-numeric, negative) returns -1.

callers treat -1 as "no body" — not an error at this layer. */
long HttpRequest::contentLength() const
{
    auto it = headers.find("content-length");
    if (it == headers.end())
        return -1;

    try
    {
        size_t pos;
        long   n = std::stol(it->second, &pos);
        /* stol consumes as many characters as form a valid integer,
        stopping at the first non-digit. pos receives the index of
        that stopping point. if pos < size(), unconsumed characters
        remain — the value is not a pure integer (e.g. "42abc").
        such values are rejected: -1 returned. */

        if (pos != it->second.size() || n < 0)
            return -1;
        return n;
    }
    catch (const std::exception&)
    {
        return -1;
    }
}

/* persistence is determined by the version default,
overridden by the Connection header if present.

HTTP/1.1 §9.3: connections are persistent by default.
HTTP/1.0: connections are not persistent by default.

RFC 9112 §9.3 / RFC 9110 §7.6.1:
  Connection: close     → override to non-persistent
  Connection: keep-alive → override to persistent

RFC 9110 §7.6.1: connection option values are case-insensitive.
header names are lowercased at parse time; values are not.
normalisation to lowercase is performed here at comparison time. */
bool HttpRequest::keepAlive() const
{
    bool persistent = (http_version == "HTTP/1.1");

    auto it = headers.find("connection");
    if (it != headers.end())
    {
        std::string val = it->second;
        std::transform(val.begin(), val.end(), val.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (val == "close")
            return false;
        if (val == "keep-alive")
            return true;
    }
    return persistent;
}
