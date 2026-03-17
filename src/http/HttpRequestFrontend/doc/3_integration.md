# http request frontend — integration


## context

this document specifies how HttpRequestFrontend integrates with
Connection and the executor. it covers:

- replacing Lukas' placeholder HttpParser
- the interface contract between components
- error response generation
- persistence (keep-alive) support
- request pipelining
- ownership model


---


## system flow

```
Connection (Lukas)
    │
    ├── owns HttpRequestFrontend (ghr)
    │       │
    │       └── advance() → ParseResult { HttpRequest }
    │
    └── calls Executor (Lukas)
            │
            ├── takes HttpRequest + ServerConfig
            ├── routing: match location, resolve path
            ├── validation: allowed_methods, upload_enable
            ├── decision: CGI or static
            └── execution: returns response string
```

3 components, 2 owners:
- ghr: HttpRequestFrontend (bytes → HttpRequest)
- Lukas: Connection (orchestration) + Executor (routing + handling)


---


## replacing the placeholder

### current state (Lukas' HttpParser)

```cpp
class HttpParser
{
    std::string buffer;
    std::string response;

public:
    void add_buffer(const Connection&, const char* buffer, ssize_t n);
    bool response_ready() const;
    std::string take_response();
};
```

Connection calls:
```cpp
http_parser.add_buffer(connection, buffer, n);
if (http_parser.response_ready())
    write_buffer = http_parser.take_response();
```

this conflates parsing with response generation.
HttpParser *is* the response — it returns a hardcoded string.
this is a placeholder to establish the event loop.

### the separation

```
bytes → HttpRequest → execute → response bytes
         ^^^^^^^       ^^^^^^^
         ghr           Lukas
```

frontend transforms bytes into structured data.
executor transforms structured data into response.
neither knows the other's internals.

### replacement contract

HttpParser is replaced by HttpRequestFrontend.
Connection code changes from:

```cpp
// before
http_parser.add_buffer(connection, buffer, n);
if (http_parser.response_ready())
    write_buffer = http_parser.take_response();
```

to:

```cpp
// after
ParseResult result = request_frontend_.advance(buffer, n);
switch (result.status)
{
    case ParseStatus::Incomplete:
        break;

    case ParseStatus::Complete:
        write_buffer = executor.execute(result.request, config);
        if (result.request.keepAlive())
            request_frontend_.reset();
        else
            state = ConnectionState::CLOSE;
        break;

    case ParseStatus::Failed:
        write_buffer = HttpResponseBuilder(result.error_code).to_string();
        state = ConnectionState::CLOSE;
        break;
}
```


---


## ownership model

Connection owns 1 HttpRequestFrontend instance per fd.

```cpp
class Connection final : public EpollHandler
{
private:
    Fd fd;
    ConnectionState state;
    HttpRequestFrontend request_frontend_;  // replaces http_parser
    std::size_t write_offset;
    std::string write_buffer;
    Listener& listener;
    // ...
};
```

lifetime: frontend constructed with Connection, destroyed with Connection.
1:1 correspondence. not shared across connections.


---


## construction and max_body_size_

frontend requires `max_body_size_` for 413 detection.
this value comes from server configuration.

```cpp
Connection::Connection(Listener& listener, int fd)
    : fd(fd)
    , state(ConnectionState::READ)
    , request_frontend_(listener.get_default_server().client_max_body_size)
    , write_offset(0)
    , listener(listener)
{
}
```

HttpRequestFrontend constructor:

```cpp
HttpRequestFrontend::HttpRequestFrontend(size_t max_body_size)
    : buffer_()
    , phase_(ParsePhase::REQUEST_LINE)
    , request_()
    , body_remaining_(0)
    , error_code_(0)
    , max_body_size_(max_body_size)
{
}
```

note: `max_body_size_` set at construction from default server config.
if virtual hosts have different limits, requires re-evaluation after
Host header parsed. current design: use default server's limit.


---


## executor interface

executor is Lukas' component. frontend provides HttpRequest.

executor's 6 steps:

```
step                  uses from HttpRequest
──────────────────────────────────────────────
match location        uri
resolve path          uri
check allowed_methods method
check upload policy   method
decide CGI/static     uri, headers
execute CGI           method, uri, headers, body
```

body is raw — no parsing by frontend. CGI receives it verbatim.

executor returns response string (via HttpResponseBuilder or directly).
executor handles application errors: 404, 405, 403, 500, 502, 504, etc.


---


## error response path

### parse errors (frontend's domain)

when parsing fails, frontend returns:
- `ParseStatus::Failed`
- `error_code` (400, 413, 501, or 505)

Connection generates error response:

```cpp
if (result.status == ParseStatus::Failed)
{
    write_buffer = HttpResponseBuilder(result.error_code).to_string();
    state = ConnectionState::CLOSE;
}
```

parse errors always close the connection.
rationale: after parse failure, stream sync is lost.
cannot reliably find next request boundary.

### application errors (executor's domain)

executor handles:
- 403 Forbidden (method not allowed, upload disabled)
- 404 Not Found
- 405 Method Not Allowed
- 500 Internal Server Error
- 502 Bad Gateway (CGI failure)
- 504 Gateway Timeout (CGI timeout)

executor returns error response via HttpResponseBuilder.
persistence decision: executor's choice per error type.


---


## persistence support

### what the frontend provides

frontend exposes data for persistence decisions.
it does not decide whether to persist.

```cpp
struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;  // "HTTP/1.0" or "HTTP/1.1"
    std::map<std::string, std::string> headers;  // keys lowercase
    std::string body;

    bool keepAlive() const;
    long contentLength() const;
};
```

### header normalisation

header names normalised to lowercase during parsing.
RFC 9110: "Field names are case-insensitive."

```cpp
// during header parsing
std::string name = /* parsed header name */;
std::transform(name.begin(), name.end(), name.begin(), ::tolower);
headers[name] = value;
```

lookup is exact match:

```cpp
auto it = headers.find("connection");
auto it = headers.find("content-length");
```

### keepAlive() specification

pure derivation from http_version and Connection header.

```cpp
bool HttpRequest::keepAlive() const
{
    if (http_version == "HTTP/1.1")
    {
        // HTTP/1.1: persistent by default
        // close only if client explicitly requests
        auto it = headers.find("connection");
        if (it != headers.end() && it->second == "close")
            return false;
        return true;
    }

    if (http_version == "HTTP/1.0")
    {
        // HTTP/1.0: not persistent by default
        // keep-alive only if client explicitly requests
        auto it = headers.find("connection");
        if (it != headers.end() && it->second == "keep-alive")
            return true;
        return false;
    }

    // unknown version: close
    return false;
}
```

note: Connection header value comparison should be case-insensitive.
implementation: normalise value to lowercase during comparison.

### contentLength() specification

convenience accessor.

```cpp
long HttpRequest::contentLength() const
{
    auto it = headers.find("content-length");
    if (it == headers.end())
        return -1;

    try
    {
        long value = std::stol(it->second);
        if (value < 0)
            return -1;
        return value;
    }
    catch (...)
    {
        return -1;
    }
}
```

returns -1 if absent or malformed.
note: malformed Content-Length triggers 400 during parsing.
this accessor is post-parse convenience.

### what the frontend does NOT do

- decide whether to keep connection open
- track connection state across requests
- manage fd lifecycle
- know persistence config
- set response headers


---


## request pipelining

HTTP/1.1 allows clients to send multiple requests without waiting
for responses. bytes from request N+1 may arrive while request N
is being processed.

### how it works

1. client sends: `request1 | request2 | request3`
2. frontend parses request1, returns Complete
3. buffer_ contains leftover bytes (start of request2)
4. Connection calls `reset()`
5. reset() clears parse state but preserves buffer_
6. next `advance()` continues from existing buffer_

### reset() semantics

```cpp
void HttpRequestFrontend::reset()
{
    // buffer_ NOT cleared — contains pipelined bytes
    phase_ = ParsePhase::REQUEST_LINE;
    request_ = HttpRequest{};
    body_remaining_ = 0;
    error_code_ = 0;
    // max_body_size_ unchanged
}
```

### pipeline processing

after completing a request:

```cpp
if (result.request.keepAlive())
{
    request_frontend_.reset();
    // next read event processes buffered bytes
}
```

lazy processing: wait for next epoll event.
buffered bytes processed on next `advance()` call.


---


## complete integration example

```cpp
void Connection::handle_read()
{
    char buffer[4096];
    ssize_t n = read(fd.get(), buffer, sizeof(buffer));

    if (n <= 0)
    {
        state = ConnectionState::CLOSE;
        return;
    }

    ParseResult result = request_frontend_.advance(buffer, n);

    switch (result.status)
    {
        case ParseStatus::Incomplete:
            break;

        case ParseStatus::Complete:
        {
            const ServerConfig& config = get_server_config(
                result.request.headers.count("host")
                    ? result.request.headers.at("host")
                    : "");

            write_buffer = executor.execute(result.request, config);
            write_offset = 0;
            state = ConnectionState::WRITE;

            if (result.request.keepAlive())
                request_frontend_.reset();
            else
                keep_alive = false;
            break;
        }

        case ParseStatus::Failed:
            write_buffer = HttpResponseBuilder(result.error_code).to_string();
            write_offset = 0;
            state = ConnectionState::WRITE;
            keep_alive = false;
            break;
    }
}

void Connection::handle_write()
{
    ssize_t n = write(fd.get(),
                      write_buffer.data() + write_offset,
                      write_buffer.size() - write_offset);

    if (n <= 0)
    {
        state = ConnectionState::CLOSE;
        return;
    }

    write_offset += n;

    if (write_offset >= write_buffer.size())
    {
        if (keep_alive)
            state = ConnectionState::READ;
        else
            state = ConnectionState::CLOSE;
    }
}
```


---


## interface summary

### HttpRequestFrontend provides

```cpp
struct HttpRequestFrontend
{
    explicit HttpRequestFrontend(size_t max_body_size);

    ParseResult advance(const char* data, size_t len);
    void reset();
};

enum class ParseStatus { Incomplete, Complete, Failed };

struct ParseResult
{
    ParseStatus status;
    HttpRequest request;    // valid iff Complete
    uint16_t    error_code; // valid iff Failed
};
```

### HttpRequest provides

```cpp
struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;

    bool keepAlive() const;
    long contentLength() const;
};
```

### Connection uses

- `advance()` on each read
- `reset()` after successful request iff `keepAlive()`
- `HttpResponseBuilder(error_code)` on parse failure
- `executor.execute(request, config)` for application logic


---


## migration checklist

1. add HttpRequest struct to `inc/http/HttpRequest.hpp`
2. add ParseStatus, ParseResult, PhaseResult, ParsePhase enums
3. implement HttpRequestFrontend with `advance()` and `reset()`
4. implement `keepAlive()` and `contentLength()` on HttpRequest
5. Lukas: implement executor with 6-step logic
6. modify Connection:
   - replace `HttpParser` with `HttpRequestFrontend`
   - update constructor to pass `max_body_size`
   - integrate executor call
7. remove HttpParser (placeholder)
