# http request frontend — overview


## ontology

bytes arrive incrementally over a socket.
the complete request is not available at once.
between `read()` calls, something must remember parse progress.
this is irreducible state.

```
ConfigFrontend:      parse : String → Config                (pure)
HttpRequestFrontend: feed  : Self × Bytes → Self × Result   (stateful)
```
    NB (ghr): see:
        thread: 20260313-0_persistence_http-request-frontend
        section: 2. Type notation
    creation of documentation upcoming


ConfigFrontend receives complete input, produces complete output,
holds no state between calls. a namespace containing a pure fn
is honest to this nature.

HttpRequestFrontend receives partial input, may produce output,
preserves state for next call. a struct holding state and exposing
methods is honest to this nature.


---


## the suspended computation

each `feed()` call advances a parse that may span many invocations.
the internal state encodes: "given what we've seen, what remains?"

functionally:

```
data ParseState = Accumulating Buffer Phase | Complete HttpRequest | Error Code

step : ParseState × Bytes → ParseState
```

practically: Connection owns one instance per fd. methods are
transformations of (self + input) → (self + result).


---


## position in system

```
phase 1: CONFIG FRONTEND    config file → ServerConfig[]     (startup)
phase 2: RUNTIME            event loop, epoll, connections
    └── per request:
        a. REQUEST FRONTEND     bytes → HttpRequest
        b. ROUTER               request + config → handler
        c. HANDLER              → output
        d. RESPONSE FRONTEND    output → bytes
```

the request frontend owns phase 2a.
hands structured request to the router.


---


## integration point: Connection ↔ HttpRequestFrontend

Connection currently uses a placeholder `HttpParser` that accumulates
bytes and returns a hardcoded response. HttpRequestFrontend replaces this.

```cpp
// Connection owns:
HttpRequestFrontend http_frontend_;

// Connection::handle_event calls:
FeedResult result = http_frontend_.feed(buf, n);

switch (result.status) {
    case FeedStatus::NeedMore:
        return;  // back to epoll, wait for more bytes
    case FeedStatus::Complete:
        route(result.request);
        if (result.request.keepAlive())
            http_frontend_.reset();  // prepare for next request
        break;
    case FeedStatus::Error:
        send_error_response(result.error_code);
        break;
}
```

the frontend knows nothing of fds, epoll, or connection lifecycle.
it receives bytes, returns status. Connection owns the event loop
integration.


---


## interface contract


### types

```cpp
enum class FeedStatus { NeedMore, Complete, Error };

struct FeedResult
{
    FeedStatus  status;
    HttpRequest request;     // valid iff status == Complete
    uint16_t    error_code;  // valid iff status == Error (400, 413, 501)
};

struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;               // "HTTP/1.0" or "HTTP/1.1"
    std::map<std::string, std::string> headers;  // keys normalised to lowercase
    std::string body;

    bool keepAlive() const;     // pure derivation from version + Connection header
    long contentLength() const; // -1 if absent or malformed
};
```


### methods

```cpp
struct HttpRequestFrontend
{
    FeedResult feed(const char* data, size_t len);
    void reset();
};
```

`feed()`: append bytes to internal buffer, advance parse.
returns as soon as status is determinable.

`reset()`: clear state for next request on persistent connection.
called by Connection after response sent, iff keepAlive() is true.


### input assumptions

bytes may arrive in arbitrary chunks. chunk boundaries carry no
semantic meaning — a chunk may split mid-method-name, mid-header,
mid-body. the frontend handles all boundary positions.


### output guarantees

on `Complete`: HttpRequest is fully populated. method is one of
GET, POST, DELETE. headers map is complete. body contains exactly
Content-Length bytes (or is empty if no body).

on `Error`: error_code is an HTTP status code suitable for response.
400 = malformed request. 413 = body exceeds limit. 501 = unknown method.

on `NeedMore`: internal state preserved. next `feed()` call continues
from where this one stopped.


---


## what the frontend does

1. accumulate bytes into internal buffer
2. parse request-line: method SP uri SP version CRLF
3. parse headers: name: value CRLF, until empty line
4. parse body: exactly Content-Length bytes (if present)
5. validate: method known, headers well-formed, body complete
6. produce HttpRequest or error code


---


## what the frontend does not do

- read bytes from fd (Connection's job)
- decide whether to keep connection open (runtime's job)
- manage fd lifecycle (runtime's job)
- route requests (Router's job)
- generate responses (Response Frontend's job)


---


## struct rationale

why struct, not namespace?

state must persist across calls. Connection owns one instance per fd.
the struct makes state visible and testable.

```cpp
struct HttpRequestFrontend
{
    // state
    std::string buffer_;
    ParsePhase phase_;
    HttpRequest request_;       // being built incrementally
    size_t body_remaining_;     // bytes still expected

    // interface
    FeedResult feed(const char* data, size_t len);
    void reset();
};
```

no encapsulation theatre. all members could be public — the struct
exists to bundle related state, not to hide it.


### comparison with ConfigFrontend

|           | ConfigFrontend        | HttpRequestFrontend |
| input     | complete file         | chunked bytes over time |
| state across calls | none         | buffer + phase + partial request |
| output    | always complete       | may be NeedMore |
| lifetime  | single invocation     | persists across N feed() calls |
| structure | namespace (process)   | struct (stateful machine) |

ConfigFrontend's `Frontend` struct exists within a single call to
`parse()`. it is constructed, used, destroyed — no external reference.
the namespace exposes a pure function; the struct is internal.

HttpRequestFrontend's struct is owned by Connection, persists across
multiple `feed()` calls, and must be externally accessible.
the struct is the interface.


---


## fragment architecture

implementation complexity warrants decomposition. the struct is
defined in one compilation target; method definitions are split
across fragment files included into that target.

```
HttpRequestFrontend.hpp     public interface (struct declaration)
HttpRequestFrontend.cpp     compilation target, includes fragments
fragments.inc
```

fragment files contain method definitions only. no includes, no
guards. they compile as part of HttpRequestFrontend.cpp's TU.


---


## persistence support

the frontend exposes data the runtime needs for persistence decisions.
see `persistence-support.md` for full specification.

summary:
- `http_version` field: HTTP/1.1 defaults to persistent, HTTP/1.0 does not
- `headers["connection"]`: client can override default
- `keepAlive()` method: pure derivation from version + header
- `contentLength()` method: required for body boundary detection

the frontend does not decide whether to persist. it exposes the data.
the runtime (Connection) calls `keepAlive()` after response is sent,
combines with response status, and decides.


---


## error semantics

parse errors produce error codes, not exceptions.
the caller (Connection) must handle errors — it cannot ignore them.

```cpp
FeedResult result = frontend.feed(buf, n);
if (result.status == FeedStatus::Error)
    send_error_response(result.error_code);
```

error codes map directly to HTTP status codes:
- 400 Bad Request: malformed request-line, malformed header, invalid method
- 413 Content Too Large: body exceeds client_max_body_size
- 501 Not Implemented: unknown HTTP method

this follows the pattern established in ConfigFrontend:
parse-time errors are reported with line numbers; semantic errors with context.
here, errors are reported with HTTP status codes — the appropriate
language for the HTTP protocol.
