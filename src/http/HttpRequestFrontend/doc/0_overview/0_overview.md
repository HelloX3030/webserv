# http request frontend — overview

## ontology

a stateful parser that transforms incrementally-arriving bytes into
a structured HTTP request.

bytes arrive over a socket in arbitrary chunks. the complete request
is not available at once. between `read()` calls, something must
remember parse progress. this is irreducible state.


in type-theoretic notation:
```
ConfigFrontend:         parse   :   String          →    Config ∪ Error  (pure)
HttpRequestFrontend:    advance :   Self × Bytes    →    Self × Result   (stateful)
```

ConfigFrontend receives complete input, produces complete output,
holds no state between calls. a namespace containing a pure fn is honest here.

HttpRequestFrontend receives partial input, may produce output,
preserves state for the next call.
The output may be Incomplete — meaning the function cannot yet produce a result.
This is partiality over the semantic output (HttpRequest), even though the function itself terminates.
a struct holding state and exposing methods is honest to this nature.


the difference reflects a fundamental distinction:
total functions over complete input vs partial functions over streaming input.*

    * total vs partial functions:
    from domain theory and denotational semantics.
    A total function is defined for all inputs in its domain — it always terminates with a value.
    A partial function may be undefined for some inputs — it may diverge, or require more input to produce output.

    a central distinction in type theory.
    e.g. in Agda:
    Total functions are the default. Every function must terminate.
    Partial functions require explicit handling: coinduction, partiality monad, or fuel.

---


## the suspended computation

each `advance()` call continues a parse that may span many invocations.
internal state encodes: "given what we've seen, what remains?"

```
data    ParseState = Accumulating Buffer Phase | Complete HttpRequest | Error Code **

step : ParseState × Bytes → ParseState ***
```


Connection owns 1 instance per fd. methods are transformations of
`(self, input) → (self, result)`.


the computation is suspended between calls. the struct is the suspension.   ****


**
    ParseState notation: Haskell algebraic data type syntax

        `data` keyword introduces a sum type.
        `=` separates the type name from its constructors.
        `|` separates variants (disjoint union).

    Reading:
    ParseState is a type with 3 constructors.
    Accumulating Buffer Phase — a constructor taking 2 arguments.
    Complete HttpRequest — a constructor taking 1 argument.
    Error Code — a constructor taking 1 argument.

    The C++ approximation would be:
    `std::variant<Accumulating, Complete, Error>`
    with each variant holding its payload.


***
    step notation: type-theoretic / mathematical notation:

        `step` is the name
        `:` means "has type"
        `×` is product (pair/tuple)
        `→` is function arrow

    In Agda this would be: `step : ParseState × Bytes → ParseState`
    In Haskell: `step :: (ParseState, Bytes) -> ParseState`
    In C++: `ParseState step(ParseState, Bytes)`


    META:
    This documentation mixes notations.
    For consistency, I should pick one: Agda-style throughout, or Haskell-style throughout.
    Mixing with C-like prototypes creates confusion.


****

    more precisely:
    Between advance() calls, the computation halts mid-parse.
    The struct's fields — buffer_, phase_, request_, body_remaining_
    — encode exactly the information needed to resume.
    This is a continuation reified as data:
    the "what remains to do" is captured in phase_,
    the "what we've accumulated" is captured in buffer_ and request_.
    Each advance() call is a step function that transforms this reified continuation.

    technical term is defunctionalisation:
    representing a suspended computation as data rather than as a closure.
    In languages with first-class continuations (Scheme, Haskell with Cont),
    you'd capture the continuation directly.
    In C++, you manually encode it as struct fields.


---


## position in system - dependency graph

```
                    ┌────────────────┐
                    │  CONFIG FILE   │
                    └───────┬────────┘
                            │ ConfigFrontend::parse()
                            ▼
                    ┌────────────────┐
                    │ ServerConfig[] │
                    └───────┬────────┘
                            │
              ┌─────────────┴─────────────┐
              │         RUNTIME           │
              │  (event loop, epoll, fds) │
              └─────────────┬─────────────┘
                            │ per connection
                            ▼
              ┌───────────────────────────────────┐
              │  HttpRequestFrontend::advance()   │
              │  dispatch → handler → response    │
              └───────────────────────────────────┘
```

archetypal flow for request-response protocols:

1. deserialise request
2. route to handler
3. execute handler
4. serialise response

(HTTP, SMTP, DNS, RPC - all follow this)



concretely in WebServ:

1. REQUEST FRONTEND       bytes → HttpRequest
2. dispatch               method → handler
3. HANDLER                → response data
4. RESPONSE BUILDER       data → bytes




Ownership in WebServ team project:

1. ghr
2. Lukas
3. Lukas
4. ?? ghr originally planned,
Lukas originally rejected to have this as individual component,
but since 20260317 has recognised its necessity...
and would like to own




---


## language-theoretic classification

HTTP/1.1 request syntax is type 3 (regular) in the Chomsky hierarchy.
see `2_grammar.md` for the formal specification.

consequences:
- no stack required (no nesting, no recursion)
- finite automaton suffices (state machine with phases)
- O(n) in input length, O(1) auxiliary space  *

the 1 context-sensitive aspect — Content-Length determining body size —
is semantic, not syntactic. handled at the HEADERS → BODY transition
by computing `body_remaining_` from the parsed header value.

the "parser" is effectively a phased scanner.
recursive descent would work but is unnecessary.
see `1_decisions/0_lang-processing/` for detailed reasoning.


*
    O(n) time: execution time grows linearly with input size

    O(1) auxiliary space:
    memory usage beyond the input and output is constant, regardless of input size


    For the HTTP parser:

        Input: the byte stream (size n)
        Output: HttpRequest (proportional to input — headers, body)
        Auxiliary space: the extra memory used during parsing

    A recursive descent parser for a deeply nested grammar might use O(d) stack space
    where d is nesting depth. A CYK parser uses O(n²) table space.

    The HTTP parser uses O(1) auxiliary space because:

        no recursion (no stack growth)
        no parse table (no dynamic allocation proportional to input)
        only fixed-size state: phase_, body_remaining_, error_code_

    The buffer grows with input, but that's the input itself, not auxiliary.




---


## interface

see `inc/http/HttpRequest.hpp` for the HttpRequest struct.
see `inc/http/HttpRequestFrontend.hpp` for ParseResult, ParseStatus, ParsePhase.


---


## input assumptions

bytes may arrive in arbitrary chunks.
chunk boundaries carry no semantic meaning -
a chunk may split mid-method, mid-header-name, mid-body.

the frontend handles all boundary positions identically:
accumulate, attempt phase completion, return or continue.

e.g.:

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
