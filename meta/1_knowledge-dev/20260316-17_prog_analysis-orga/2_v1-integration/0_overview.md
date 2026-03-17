# v1 integration — overview

minimal adaptation to existing infrastructure.


---


## ghr's deliverables

1. `HttpRequest` type — data struct, no methods
2. `HttpRequestFrontend` — stateful parser, bytes → HttpRequest
3. interface contract with Connection


---


## HttpRequest type

```cpp
struct HttpRequest
{
    std::string method;                           // "GET", "POST", "DELETE"
    std::string uri;                              // "/path/to/resource"
    std::string http_version;                     // "HTTP/1.0" or "HTTP/1.1"
    std::map<std::string, std::string> headers;   // keys normalised to lowercase
    std::string body;                             // Content-Length bytes or empty
};
```

pure data. no behaviour.
produced by HttpRequestFrontend.
consumed by dispatch glue in Connection.


---


## HttpRequestFrontend interface

```cpp
enum class ParseStatus { Incomplete, Complete, Failed };

struct ParseResult
{
    ParseStatus status;
    HttpRequest request;     // valid iff Complete
    uint16_t    error_code;  // valid iff Failed (400, 413, 501)
};

struct HttpRequestFrontend
{
    ParseResult advance(const char* data, size_t len);
    void reset();
};
```

`advance()`: append bytes, advance parse, return status.
`reset()`: clear state for next request (keep-alive).


---


## Connection integration

current (placeholder):

```cpp
http_parser.add_buffer(connection, buffer, n);
if (http_parser.response_ready())
    write_buffer = http_parser.take_response();
```

after ghr's frontend:

```cpp
ParseResult result = request_frontend.advance(buffer, n);

switch (result.status)
{
    case ParseStatus::Incomplete:
        return;  // back to epoll

    case ParseStatus::Complete:
        write_buffer = dispatch(result.request);
        break;

    case ParseStatus::Failed:
        write_buffer = HttpResponseBuilder(result.error_code).to_string();
        break;
}
```

ghr provides the frontend & Lukas implements the switch.


---


## dispatch glue

in Connection (Lukas's responsibility):

```cpp
std::string dispatch(const HttpRequest& req)
{
    const ServerConfig& config = get_server_config(req.headers["host"]);
    HttpResponseBuilder response;

    if (req.method == "GET")
        response = WebServ::http_get(config, req.uri);
    else if (req.method == "POST")
        response = WebServ::http_post(config, req.uri, req.body);
    else if (req.method == "DELETE")
        response = WebServ::http_delete(config, req.uri);
    else
        response = HttpResponseBuilder(501);

    return response.to_string();
}
```

handlers unchanged — still take (config, path, body?).
no Router. routing logic remains in handlers.


---


## error response path

parse errors (400, 413, 501) bypass handlers entirely.

```cpp
case ParseStatus::Failed:
    write_buffer = HttpResponseBuilder(result.error_code).to_string();
    break;
```

no custom error pages for parse errors (simplification).
custom error pages only for handler-level errors (404, 405, 403, 500).

Lukas can add error page lookup later if desired.


---


## what ghr does NOT do

- Router — rejected
- HttpResponseFrontend — `to_string()` already exists
- error page lookup — impure, Lukas's concern
- Connection modifications — Lukas's code
- handler refactoring — Lukas's code


---


## deliverable checklist

- [ ] `HttpRequest` struct in `inc/http/HttpRequest.hpp`
- [ ] `ParseStatus`, `ParseResult` in `inc/http/HttpRequestFrontend.hpp`
- [ ] `HttpRequestFrontend` struct in `inc/http/HttpRequestFrontend.hpp`
- [ ] implementation in `src/http/HttpRequestFrontend/`
- [ ] documentation in `src/http/HttpRequestFrontend/`
