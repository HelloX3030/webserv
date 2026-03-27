#pragma once

#include <map>
#include <string>

/*
structured request.
upon parsing completion,
passed downstream to `Connection`.

fields populated by HttpRequestFrontend.
*/
struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;

    bool keepAlive() const;

    /* convenience accessor for Content-Length header.
    -1 if absent or malformed. */
    long contentLength() const;
    std::string to_string() const;
};
