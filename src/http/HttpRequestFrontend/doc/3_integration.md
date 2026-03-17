# http request frontend — integration


## context

this document specifies how HttpRequestFrontend integrates with
Connection and the broader runtime. it covers:

- replacing Lukas' placeholder HttpParser
- the interface contract between Connection and frontend
- dispatch to handlers
- error response generation
- persistence (keep-alive) support
- request pipelining
- ownership model


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
bytes → HttpRequest → dispatch → handle → response bytes
         ^^^^^^^
         frontend's job
```

the frontend transforms bytes into structured data.
it knows nothing of routing, handling, or response generation.

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
        // wait for more bytes
        break;

    case ParseStatus::Complete:
        write_buffer = dispatch(result.request);
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

lifetime: frontend is constructed when Connection is constructed,
destroyed when Connection is destroyed. 1:1 correspondence.

the frontend is not shared across connections.
each connection has independent parse state.


---


## construction and max_body_size_

the frontend requires `max_body_size_` for 413 detection.
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

note: `max_body_size_` is set once at construction.
if virtual hosts have different limits, this requires resolution.
current design: use default server's limit.
future: re-evaluate after Host header parsed (complicates state machine).


---


## dispatch

dispatch is Connection's responsibility, not the frontend's.

the frontend outputs HttpRequest. Connection routes to handlers.

```cpp
std::string Connection::dispatch(const HttpRequest& request)
{
    const ServerConfig& config = get_server_config(request.headers.at("host"));

    if (request.method == "GET")
        return HttpMethods::http_get(config, request.uri);
    if (request.method == "POST")
        return HttpMethods::http_post(config, request.uri, request.body);
    if (request.method == "DELETE")
        return HttpMethods::http_delete(config, request.uri);

    // unreachable: frontend rejects unknown methods with 501
    return HttpResponseBuilder(500).to_string();
}
```

note: HttpMethods functions currently return response strings.
they contain embedded routing logic. this is existing structure —
frontend integration does not change it.


---


## error response path

when parsing fails, the frontend returns:
- `ParseStatus::Failed`
- `error_code` (400, 413, 501, or 505)

Connection generates the error response:

```cpp
if (result.status == ParseStatus::Failed)
{
    write_buffer = HttpResponseBuilder(result.error_code).to_string();
    state = ConnectionState::CLOSE;
}
```

error responses always close the connection.
rationale: after a parse error, stream synchronisation is lost.
we cannot reliably find the next request boundary.

HttpResponseBuilder already supports status-only construction:

```cpp
explicit HttpResponseBuilder(int status);
```

this generates a minimal response with status line and empty body.


---


## persistence support

### what the frontend provides

the frontend exposes data for persistence decisions.
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

header names are normalised to lowercase during parsing.
RFC 9110: "Field names are case-insensitive."

```cpp
// during header parsing
std::string name = /* parsed header name */;
std::transform(name.begin(), name.end(), name.begin(), ::tolower);
headers[name] = value;
```

lookup is then exact match:

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
RFC 9110 section 7.6.1: connection options are case-insensitive.
implementation detail: normalise value to lowercase, or use
case-insensitive comparison.

### contentLength() specification

convenience accessor for Content-Length header.

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

returns -1 if header absent or malformed.
note: during parsing, malformed Content-Length triggers 400.
this accessor is for post-parse convenience, not validation.

### what the frontend does NOT do

- decide whether to keep connection open (Connection's job)
- track connection state across requests (Connection's job)
- manage fd lifecycle (Connection's job)
- know whether persistence is enabled in config (doesn't need to)
- set response headers (HttpResponseBuilder's job)

the frontend's only persistence-related responsibility:
expose the data correctly.


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
6. next `advance()` call continues from existing buffer_ bytes

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

### Connection's pipeline loop

after completing a request:

```cpp
if (result.request.keepAlive())
{
    request_frontend_.reset();
    // immediately try to parse next request from buffered bytes
    result = request_frontend_.advance("", 0);  // empty input, process buffer
    // handle result...
}
```

or simply wait for next epoll event — the buffered bytes will be
processed on the next `advance()` call with new input appended.

design choice: eager vs lazy pipeline processing.
eager: call `advance("", 0)` immediately after reset.
lazy: wait for next read event.

lazy is simpler. eager reduces latency for pipelined requests.
current recommendation: lazy. optimise later if needed.


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
            // remain in READ state, wait for more bytes
            break;

        case ParseStatus::Complete:
            write_buffer = dispatch(result.request);
            write_offset = 0;
            state = ConnectionState::WRITE;

            if (result.request.keepAlive())
                request_frontend_.reset();
            else
                keep_alive = false;  // close after write completes
            break;

        case ParseStatus::Failed:
            write_buffer = HttpResponseBuilder(result.error_code).to_string();
            write_offset = 0;
            state = ConnectionState::WRITE;
            keep_alive = false;  // always close on error
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
        // write complete
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
    HttpRequestFrontend(size_t max_body_size);

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
- `dispatch()` to route successful requests to handlers


---


## migration checklist

1. add HttpRequest struct to `inc/http/HttpRequest.hpp`
2. add ParseStatus, ParseResult, PhaseResult enums
3. implement HttpRequestFrontend with `advance()` and `reset()`
4. implement `keepAlive()` and `contentLength()` on HttpRequest
5. modify Connection:
   - replace `HttpParser http_parser` with `HttpRequestFrontend request_frontend_`
   - update constructor to pass `max_body_size`
   - replace `add_buffer` / `response_ready` / `take_response` with new flow
6. implement `dispatch()` in Connection
7. remove HttpParser (placeholder)
