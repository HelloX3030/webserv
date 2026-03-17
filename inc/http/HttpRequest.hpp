#pragma once

#include <algorithm>
#include <map>
#include <string>

/*
the structured request passed to executor after parsing completes.
fields populated by HttpRequestFrontend. body is raw — no interpretation.
*/
struct HttpRequest
{
    std::string                        method;       // "GET", "POST", "DELETE"
    std::string                        uri;          // "/path/to/resource?query=value"
    std::string                        http_version; // "HTTP/1.0", "HTTP/1.1"
    std::map<std::string, std::string> headers;      // keys normalised to lowercase
    std::string                        body;         // raw bytes, exactly Content-Length

    // derive persistence from http_version and Connection header.
    // HTTP/1.1: persistent by default, close if Connection: close
    // HTTP/1.0: not persistent by default, persist if Connection: keep-alive
    bool keepAlive() const;

    // convenience accessor for Content-Length header.
    // returns -1 if absent or malformed.
    long contentLength() const;
};
