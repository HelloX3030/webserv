# current state — overview

what exists, who owns what.


---


## ownership boundaries

```
ghr                             Lukas
───                             ─────
ConfigFrontend                  runtime (core/, net/)
  config file → ServerConfig      event loop, epoll, fd lifecycle

HttpRequestFrontend             Connection
  bytes → HttpRequest             owns fd, read/write buffers
  (in progress)                   calls parser, dispatches response

                                HttpParser (placeholder)
                                  accumulates bytes
                                  returns hardcoded response

                                HttpMethods (handlers)
                                  http_get, http_post, http_delete
                                  contain routing + execution

                                HttpResponseBuilder
                                  accumulates status, headers, body
                                  serializes via to_string()
```


---


## data flow — actual current path

```
bytes arrive
    │
    ▼
Connection::handle_event(EPOLLIN)
    │
    ▼
HttpParser::add_buffer()
    │  accumulates bytes
    │  sets hardcoded response (placeholder)
    │
    ▼
HttpParser::response_ready() → true
    │
    ▼
HttpParser::take_response() → write_buffer
    │
    ▼
Connection::handle_event(EPOLLOUT)
    │
    ▼
bytes out
```

HttpParser is a stub - no actual parsing occurs.
response is generated immediately on any input.


---


## data flow — intended path (after ghr's frontend)

```
bytes arrive
    │
    ▼
Connection::handle_event(EPOLLIN)
    │
    ▼
HttpRequestFrontend::advance()
    │
    ├─ Incomplete → return, wait for more bytes
    │
    ├─ Complete → HttpRequest
    │       │
    │       ▼
    │   dispatch on method
    │       │
    │       ▼
    │   http_get/post/delete(config, uri, body?)
    │       │  routing + execution (conflated)
    │       │
    │       ▼
    │   HttpResponseBuilder
    │       │
    │       ▼
    │   to_string() → write_buffer
    │
    └─ Failed → error_code
            │
            ▼
        HttpResponseBuilder(code).to_string() → write_buffer
```


---


## key types

### HttpParser (Lukas, placeholder)

```cpp
class HttpParser {
    std::string buffer;
    std::string response;

    void add_buffer(Connection&, const char*, ssize_t);
    bool response_ready() const;
    std::string take_response();
};
```

conflates parsing with response generation.
`add_buffer` sets `response` to a hardcoded string.
will be replaced by HttpRequestFrontend.


### HttpResponseBuilder (Lukas)

```cpp
class HttpResponseBuilder {
    int status;
    std::string body;
    std::map<std::string, std::string> headers;

    void set_status(int);
    void set_body(const std::string&);
    void set_header(const std::string&, const std::string&);
    std::string to_string() const;  // serialization
};
```

conflates data with serialization.
handlers return this directly.
`to_string()` produces wire-format bytes.


### HttpMethods (Lukas, handlers)

```cpp
namespace WebServ {
    HttpResponseBuilder http_get(const ServerConfig&, const std::string& path);
    HttpResponseBuilder http_post(const ServerConfig&, const std::string& path,
                                  const std::string& content);
    HttpResponseBuilder http_delete(const ServerConfig&, const std::string& path);
}
```

each handler contains:
- location matching (duplicated, ~15 lines each)
- method checking (duplicated)
- path resolution + traversal check (duplicated)
- actual file I/O (unique per method)
- response construction

routing and execution conflated.


---


## missing types

### HttpRequest (ghr, to be defined)

```cpp
struct HttpRequest {
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
};
```

output of HttpRequestFrontend, input to dispatch logic.
does not currently exist in codebase.
