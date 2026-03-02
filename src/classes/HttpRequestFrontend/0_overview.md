# http request frontend — overview


## essence

state machine:
```
bytes (incremental) → Complete(HttpRequest) | NeedMore | Error(code)
```

not a pure function — maintains parse state across calls.
fed bytes by Connection. produces structured request when complete.


---


## position in system
```
phase 1: CONFIG FRONTEND    config file → ServerConfig[]     (startup, one-shot)
phase 2: RUNTIME            event loop, poll, connections
    └── per request:
        a. REQUEST FRONTEND     bytes → HttpRequest
        b. ROUTER               request + config → handler
        c. HANDLER              → output
        d. RESPONSE FRONTEND    output → bytes
```

the request frontend owns phase 2a.
hands off structured request to the router.
```
bytes from fd
    │
    v
Connection::read_buffer
    │
    v
HttpRequestFrontend::feed()
    │
    ├─ NeedMore     → return to event loop, wait for more bytes
    ├─ Error(code)  → generate error response (400, 413, 501)
    └─ Complete
            │
            v
        HttpRequest     ← frontend's output
            │
            v
        Router
```


---


## telos

produce a validated, complete, router-ready representation
of client intent.

"router-ready": method classified, URI available, headers
accessible, body complete. the router and handlers trust the
request without further parsing.


---


## what it is not

not responsible for reading bytes from fd (Connection's job).
not responsible for routing (Router's job).
not responsible for responses (Response Frontend's job).
not responsible for connection lifecycle (runtime's job).


---


## architectural difference from ConfigFrontend

ConfigFrontend: batch processing.
```
entire file available → single call → result
```

HttpRequestFrontend: stream processing.
```
bytes arrive in chunks → multiple calls → state transitions
```

ConfigFrontend is a pure function.
HttpRequestFrontend is a state machine.

this shapes everything:
- must handle arbitrary chunk boundaries (mid-token, mid-header)
- must preserve parse state between calls
- must detect "need more data" vs "malformed input"


---


## input / output contract

input:
    byte chunk (arbitrary size, arbitrary boundaries).
    chunks may split anywhere — mid-method, mid-header-name,
    mid-body.

output:
    one of:
```cpp
enum class ParseResult { NeedMore, Complete, Error };

struct FeedResult
{
    ParseResult     status;
    HttpRequest     request;    // valid iff Complete
    uint16_t        error_code; // valid iff Error (400, 413, 501, ...)
};
```

on Complete:
    HttpRequest contains method, uri, http_version, headers, body.

on Error:
    error_code is the HTTP status to return (400 Bad Request, etc.).
    request is empty/invalid.

on NeedMore:
    call feed() again when more bytes arrive.
    frontend preserves internal state.