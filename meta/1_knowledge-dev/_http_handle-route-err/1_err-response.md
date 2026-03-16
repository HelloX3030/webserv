# interface: parse error → HTTP error response


## context

parse errors in HttpRequestFrontend must become HTTP error responses
sent to the client. this crosses 3 module boundaries:

```
HttpRequestFrontend  →  error_code  →  Runtime  →  HttpResponseFrontend  →  bytes
```

each module has a distinct responsibility.
this issue specifies the contracts between them.


## module responsibilities


### HttpRequestFrontend (Ganesha)

returns `ParseResult` on every `advance()` call:
**WILL RETURN ASAP. WORK-IN-PROGRESS**

```cpp
struct ParseResult
{
    ParseStatus status;      // Incomplete, Complete, Failed
    HttpRequest request;     // valid iff Complete
    uint16_t    error_code;  // valid iff Failed
};
```

on parse failure, `error_code` is an HTTP status code:
- 400: malformed request line, malformed header
- 413: body exceeds client_max_body_size
- 501: unknown HTTP method

the frontend does NOT:
- generate response bytes
- look up error pages
- know about ServerConfig


### Runtime (Lukas)

Connection receives ParseResult from frontend:

```cpp
ParseResult result = request_frontend_.advance(buf, n);

switch (result.status)
{
    case ParseStatus::Incomplete:
        return;  // back to epoll

    case ParseStatus::Complete:
        // route and handle request
        break;

    case ParseStatus::Failed:
        write_buffer_ = HttpResponseFrontend::error(
            result.error_code,
            get_server_config(/* host from partial parse? default? */)
        );
        // queue for write, then close or keep-alive
        break;
}
```

runtime's job:
- call frontend with bytes from read()
- on Failed, obtain ServerConfig and call response frontend
- write response bytes to fd
- manage connection lifecycle


### HttpResponseFrontend (Ganesha)

generates HTTP response bytes from structured input:

```cpp
namespace HttpResponseFrontend
{
    std::string error(uint16_t code, const ServerConfig& config);
}
```

implementation:
1. check `config.error_pages[code]` for custom page path
2. if exists: read file, use as body
3. if not: generate default error body
4. serialise to HTTP response format

uses Lukas's existing `HttpResponse` class for serialisation:

```cpp
std::string error(uint16_t code, const ServerConfig& config)
{
    HttpResponse resp(code);

    auto it = config.error_pages.find(code);
    if (it != config.error_pages.end())
    {
        std::string body = read_file(resolve_path(config, it->second));
        resp.set_body(body);
    }
    else
    {
        resp.set_body(default_error_body(code));
    }

    resp.set_header("Content-Type", "text/html");
    return resp.to_string();
}
```


## open questions

1. **host for config lookup on parse error**: if the request line is
   malformed, we may not have a Host header. which ServerConfig to use?
   likely: default server for the listener.
   The Listener knows which ServerConfigs it's bound to — the first in the list (or one marked default_server) is the fallback

2. **Connection header on error responses**: close unconditionally,
   or respect keep-alive if we parsed enough to know? conservative
   default: close. malformed request → uncertain state.

probably best:
Close unconditionally for parse errors.
The request is malformed — the byte stream may be in an unknown state.
Attempting keep-alive risks misframing the next request.

3. **HttpResponse naming**: Lukas's `HttpResponse` is really a
   serialiser/builder. consider renaming to `HttpResponseBuilder`
   to distinguish from the frontend's orchestration role.

IMPORTANT!
REMOVE IO FROM LUKAS' HTTP RESPONSE
FRONTEND IS PURE TRANSFORMATION - SERIALISATION



## acceptance criteria

- [ ] HttpRequestFrontend returns error_code on parse failure
- [ ] Runtime passes error_code + ServerConfig to response frontend
- [ ] HttpResponseFrontend generates error response bytes
- [ ] custom error pages from config.error_pages honoured
- [ ] default error body generated when no custom page configured
