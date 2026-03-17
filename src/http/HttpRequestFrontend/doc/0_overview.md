# http request frontend — overview


---


## ontology

a stateful parser that transforms incrementally-arriving bytes into
a structured HTTP request.

bytes arrive over a socket in arbitrary chunks. the complete request
is not available at once. between `read()` calls, something must
remember parse progress. this is irreducible state.


```
ConfigFrontend:      parse : String → Config              (pure)
HttpRequestFrontend: advance : Self × Bytes → Self × Result   (stateful)
```

ConfigFrontend receives complete input, produces complete output,
holds no state between calls. a namespace containing a pure function
is honest to this nature.

HttpRequestFrontend receives partial input, may produce output,
preserves state for the next call. a struct holding state and exposing
methods is honest to this nature.

the difference is not stylistic. it reflects a fundamental distinction:
total functions over complete input vs partial functions over streaming input.


---


## the suspended computation

each `advance()` call continues a parse that may span many invocations.
internal state encodes: "given what we've seen, what remains?"

```
data ParseState = Accumulating Buffer Phase | Complete HttpRequest | Error Code

step : ParseState × Bytes → ParseState
```

Connection owns 1 instance per fd. methods are transformations of
`(self, input) → (self, result)`.

the computation is suspended between calls. the struct is the suspension.


---


## position in system
```
phase 1: CONFIG FRONTEND      config file → ServerConfig[]       (startup)

phase 2: RUNTIME              event loop, epoll, connections
    └── per request:
        a. REQUEST FRONTEND       bytes → HttpRequest
        b. dispatch               method → handler
        c. HANDLER                → response data
        d. RESPONSE BUILDER       data → bytes
```

the request frontend owns phase 2a.
it:
    receives bytes from Connection.
    produces `HttpRequest` for dispatch.
    knows nothing of routing, handling, or response generation.


---


## language-theoretic classification

HTTP/1.1 request syntax is type 3 (regular) in the Chomsky hierarchy.
see `2_grammar.md` for the formal specification.

consequences:
- no stack required (no nesting, no recursion)
- finite automaton suffices (state machine with phases)
- O(n) in input length, O(1) auxiliary space

the 1 context-sensitive aspect — Content-Length determining body size —
is semantic, not syntactic. handled at the HEADERS → BODY transition
by computing `body_remaining_` from the parsed header value.

the "parser" is effectively a phased scanner.
recursive descent would work but is unnecessary.
see `1_decisions/0_lang-processing/` for detailed reasoning.


---


## interface

### types
```cpp
enum class ParseStatus { Incomplete, Complete, Failed };

struct ParseResult
{
    ParseStatus status;
    HttpRequest request;     // valid iff Complete
    uint16_t    error_code;  // valid iff Failed (400, 413, 501, 505)
};

struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;  // keys normalised to lowercase
    std::string body;

    bool keepAlive() const;      // pure derivation from version + Connection header
    long contentLength() const;  // -1 if absent or malformed
};
```

### methods
```cpp
struct HttpRequestFrontend
{
    ParseResult advance(const char* data, size_t len);
    void reset();
};
```

`advance()`: append bytes to internal buffer, advance parse state.
returns as soon as status is determinable.

`reset()`: clear state for next request on persistent connection.
called by Connection after response sent, iff `keepAlive()` is true.
buffer is not cleared — may contain bytes from pipelined next request.


---


## input assumptions

bytes may arrive in arbitrary chunks. chunk boundaries carry no semantic meaning.
a chunk may split mid-method, mid-header-name, mid-body.
the frontend handles all boundary positions identically:
accumulate, attempt phase completion, return or continue.


examples:
```
"GET /pa"           ← mid-uri
"th HTTP/1.1\r\n"   ← completes request line

"Content-Len"       ← mid-header-name
"gth: 5\r\n\r\n"    ← completes headers
"hello"             ← body


"POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\nhel"  ← request line + headers + partial body
"lo worl"           ← more body
"d"                 ← completes body
```


---


## output guarantees

**on Complete**: `HttpRequest` is fully populated.
- `method` is one of GET, POST, DELETE
- `uri` is the request target (origin-form)
- `http_version` is "HTTP/1.0" or "HTTP/1.1"
- `headers` map is complete, keys lowercase
- `body` contains exactly Content-Length bytes (or empty if no body)

**on Failed**: `error_code` is an HTTP status code.
- 400 Bad Request: malformed syntax
- 413 Content Too Large: body exceeds limit
- 501 Not Implemented: unknown method or chunked encoding
- 505 HTTP Version Not Supported: not HTTP/1.x

**on Incomplete**: internal state preserved.
next `advance()` call continues from current position.


---


## what the frontend does

1. accumulate bytes into internal buffer
2. parse request-line: method SP uri SP version CRLF
3. parse headers: name ":" value CRLF, until empty line
4. consume body: exactly Content-Length bytes (if present)
5. produce `HttpRequest` or error code


---


## what the frontend does not do

- read bytes from fd — Connection's responsibility
- manage fd lifecycle — Connection's responsibility
- decide persistence — runtime combines `keepAlive()` with response status
- route requests — dispatch logic, downstream
- generate responses — HttpResponseBuilder, downstream
- execute handlers — handler functions, downstream
- access filesystem — handlers only
- know server configuration — receives `client_max_body_size` as parameter


---


## state
```cpp
struct HttpRequestFrontend
{
    std::string buffer_;         // accumulated unparsed bytes
    ParsePhase  phase_;          // current phase
    HttpRequest request_;        // being built incrementally
    size_t      body_remaining_; // bytes still expected
    uint16_t    error_code_;     // set on ERROR transition
    size_t      max_body_size_;  // from config, for 413 detection
};
```

`buffer_` accumulates bytes across `advance()` calls.
consumed bytes are erased after each successful phase transition.

`request_` fields are populated incrementally:
method/uri/version after REQUEST_LINE, headers after each header line,
body after BODY phase completes.

`phase_` reflects current parse position. advances monotonically (except `reset()`).
see `0b_state-machine.md` for transitions.


---


## struct rationale

why struct, not namespace?

state must persist across calls. Connection owns 1 instance per fd.
the struct makes state explicit, visible, testable.

no encapsulation theatre. members could be public — the struct exists
to bundle related state, not to hide it. private members are a courtesy
to future maintainers: "these are internal, don't depend on them."

### comparison with ConfigFrontend

|                      | ConfigFrontend          | HttpRequestFrontend         |
|----------------------|-------------------------|-----------------------------|
| input                | complete file           | chunked bytes over time     |
| state across calls   | none                    | buffer + phase + partial request |
| output               | always complete         | may be Incomplete           |
| lifetime             | single invocation       | persists across N advance() |
| structure            | namespace (process)     | struct (stateful machine)   |

ConfigFrontend's internal `Frontend` struct exists within a single
call to `parse()`. constructed, used, destroyed — no external reference.
the namespace exposes a pure function; the struct is implementation detail.

HttpRequestFrontend's struct is owned by Connection, persists across
multiple `advance()` calls, must be externally accessible.
the struct is the interface.


---


## error semantics

parse errors produce error codes, not exceptions.
the caller must handle errors — cannot ignore the return value.
```cpp
ParseResult result = frontend.advance(buf, n);
if (result.status == ParseStatus::Failed)
    send_error_response(result.error_code);
```

error codes map to HTTP status codes — the appropriate language
for protocol-level failures. see `1_decisions/0_lang-processing/3_failure-response.md`.

fail-fast strategy: first error terminates parsing.
no attempt to recover or accumulate multiple errors.
protocol streams lack synchronisation points after corruption.


---


## persistence support

the frontend exposes data the runtime needs for persistence decisions.
see `3_integration.md` for the full contract.

summary:
- `http_version`: HTTP/1.1 defaults persistent, HTTP/1.0 does not
- `headers["connection"]`: client can override default
- `keepAlive()`: pure derivation from version + header

the frontend does not decide whether to persist.
it exposes the data. the runtime decides.


---


## references

RFC 9110: HTTP Semantics
RFC 9112: HTTP/1.1
see `2_grammar.md` for complete reference list.

`meta/1_knowledge-dev/language-processing/` for formal foundations.
`meta/1_knowledge-dev/network-protocols/http/` for protocol documentation. (WIP)
