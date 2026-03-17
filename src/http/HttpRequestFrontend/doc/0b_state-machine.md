# http request frontend — state machine


## the phases

```
REQUEST_LINE    parsing method, uri, version
HEADERS         parsing header lines
BODY            accumulating body bytes
COMPLETE        request fully parsed
ERROR           parse failed, terminal
```

each phase has a completion condition. when satisfied, transition
to the next phase. if malformed input detected, transition to ERROR.


---


## state

```cpp
enum class ParsePhase { REQUEST_LINE, HEADERS, BODY, COMPLETE, ERROR };

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

`buffer_` accumulates bytes across `advance()` calls. consumed bytes
are erased after each successful phase transition.

`request_` fields are populated incrementally: method/uri/version
after REQUEST_LINE completes, headers after each header line,
body after BODY completes.

`max_body_size_` is set at construction from server configuration.
used at HEADERS → BODY transition to detect 413 before body accumulation.


---


## transitions

```
                    ┌───────────────────┐
        start ────▶ │   REQUEST_LINE    │
                    └─────────┬─────────┘
                              │ CRLF found, line valid
                              ▼
                    ┌───────────────────┐
               ┌───▶│     HEADERS       │◀───┐
               │    └─────────┬─────────┘    │
               │              │              │
               │     header line valid       │
               └─────────────────────────────┘
                              │
                              │ empty line (CRLF alone)
                              ▼
                    ┌───────────────────┐
                    │       BODY        │  ← skip if Content-Length absent or 0
                    └─────────┬─────────┘
                              │ body_remaining_ == 0
                              ▼
                    ┌───────────────────┐
                    │     COMPLETE      │
                    └───────────────────┘
```

ERROR reachable from REQUEST_LINE, HEADERS, or BODY.
COMPLETE and ERROR are terminal — no further transitions.


---


## phase: REQUEST_LINE

waiting for: `Method SP URI SP HTTP-Version CRLF`

scan buffer for CRLF. if not found, return NeedMore.
if found, extract line, parse into 3 tokens separated by SP.

validation sequence:
1. split on SP — must yield exactly 3 tokens
2. method token — must be GET, POST, or DELETE
3. uri token — must begin with `/`
4. version token — must be `HTTP/1.0` or `HTTP/1.1`

success:
    populate request_.method, request_.uri, request_.http_version.
    erase consumed bytes from buffer_.
    transition → HEADERS.

failure conditions:
    missing SP, wrong token count → 400
    unknown method → 501
    malformed version → 400
    unsupported version (not HTTP/1.x) → 505


---


## phase: HEADERS

waiting for: `Header-Name: Header-Value CRLF` or empty line `CRLF`

scan buffer for CRLF. if not found, return NeedMore.

if line is empty (just CRLF):
    headers complete.
    extract Content-Length from headers (0 if absent).
    if Content-Length > max_body_size_: error 413, transition → ERROR.
    set body_remaining_ = Content-Length.
    if body_remaining_ == 0: transition → COMPLETE.
    else: transition → BODY.

if line is non-empty:
    find colon. if absent, error 400.
    extract name (before colon), value (after colon, trimmed).
    normalise name to lowercase.
    insert into request_.headers.
    erase consumed bytes from buffer_.
    remain in HEADERS.

note: 413 is detected at the HEADERS → BODY transition, not during
body accumulation. this is fail-fast: reject before allocating.


---


## phase: BODY

waiting for: `body_remaining_` bytes.

if buffer_.size() >= body_remaining_:
    extract exactly body_remaining_ bytes into request_.body.
    erase consumed bytes from buffer_.
    transition → COMPLETE.

else:
    return NeedMore.

note: buffer_ may contain more bytes than body_remaining_ if
the client pipelined requests. we consume exactly body_remaining_,
leaving the rest for the next request after reset().


---


## phase: COMPLETE

terminal. request_ is valid. advance() returns Complete.


---


## phase: ERROR

terminal. error_code_ is set. advance() returns Failed.


---


## advance() control flow

internal result type for phase-parsing functions:

```cpp
enum class PhaseResult { Advanced, NeedMore, Failed };
```

`Advanced` — phase completed, transitioned to next phase.
`NeedMore` — insufficient bytes, remain in current phase.
`Failed` — parse error, transitioned to ERROR.

```cpp
ParseResult HttpRequestFrontend::advance(const char* data, size_t len)
{
    buffer_.append(data, len);

    while (true)
    {
        switch (phase_)
        {
            case ParsePhase::REQUEST_LINE:
            {
                PhaseResult r = parse_request_line();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                    return {ParseStatus::Failed, {}, error_code_};
                break;  // Advanced — loop continues
            }

            case ParsePhase::HEADERS:
            {
                PhaseResult r = parse_header_line();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                    return {ParseStatus::Failed, {}, error_code_};
                break;
            }

            case ParsePhase::BODY:
            {
                PhaseResult r = consume_body();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                    return {ParseStatus::Failed, {}, error_code_};
                break;
            }

            case ParsePhase::COMPLETE:
                return {ParseStatus::Complete, request_, 0};

            case ParsePhase::ERROR:
                return {ParseStatus::Failed, {}, error_code_};
        }
    }
}
```

the while loop handles the case where a single `advance()` call
provides enough bytes to complete multiple phases. we loop until
we hit NeedMore, Complete, or Failed.

each `parse_*` function returns a 3-valued result. control flow
is explicit at every call site. no implicit state inspection.


---


## error codes

| condition | code | phase |
|-----------|------|-------|
| malformed request line (wrong token count, bad SP) | 400 | REQUEST_LINE |
| empty or invalid uri | 400 | REQUEST_LINE |
| malformed version string | 400 | REQUEST_LINE |
| unknown method (not GET/POST/DELETE) | 501 | REQUEST_LINE |
| unsupported version (not HTTP/1.x) | 505 | REQUEST_LINE |
| malformed header (missing colon) | 400 | HEADERS |
| Content-Length non-numeric or negative | 400 | HEADERS |
| Content-Length exceeds max_body_size_ | 413 | HEADERS |
| chunked Transfer-Encoding (not implemented) | 501 | HEADERS |


---


## reset()

called by Connection after response sent, iff keepAlive() is true.

```cpp
void HttpRequestFrontend::reset()
{
    // buffer_ may contain bytes from next request — do not clear
    phase_ = ParsePhase::REQUEST_LINE;
    request_ = HttpRequest{};
    body_remaining_ = 0;
    error_code_ = 0;
    // max_body_size_ unchanged — same config for connection lifetime
}
```

buffer_ is not cleared. pipelined requests leave trailing bytes
after the current request's body. these bytes belong to the next
request and must survive the reset.

max_body_size_ is not reset — it derives from server configuration
and remains constant for the connection's lifetime.


---


## invariants

1. buffer_ contains all unparsed bytes. consumed bytes are erased.

2. phase_ reflects current parse position.
    only advances, never retreats (except reset()).

3. request_ fields are valid for completed phases only.
   method/uri/version valid after REQUEST_LINE.
   headers valid after HEADERS.
   body valid after BODY (or empty if no body).

4. body_remaining_ is computed from Content-Length at HEADERS → BODY
   transition. decremented as bytes are consumed. 0 when BODY → COMPLETE.

5. error_code_ is 0 unless phase_ is ERROR.

6. max_body_size_ is constant after construction.


---


## chunk boundary handling

bytes arrive at arbitrary boundaries. examples:

```
"GET /pa"           ← mid-uri
"th HTTP/1.1\r\n"   ← completes request line
```

```
"Content-Len"       ← mid-header-name
"gth: 5\r\n\r\n"    ← completes header + empty line
"hello"             ← body
```

```
"POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\nhel"
                    ← request line + headers + partial body
"lo worl"           ← more body
"d"                 ← completes body
```

the state machine handles all cases identically: accumulate into
buffer_, scan for phase completion, consume and transition or
return NeedMore.


---


## language perspectives

Agda — incremental parsing modelled as a coinductive type or
a state machine with explicit fuel. the mathematical structure
is the same: `ParseState → Input → ParseState × Maybe Result`.


Haskell — attoparsec provides incremental parsing via `parse` and `feed`.
partial results carry continuation state.
the `IResult` type is `Fail | Partial (ByteString -> IResult) | Done remaining result`.
same 3-valued outcome: failed, need more, complete.


Rust — nom's streaming parsers return `Incomplete(Needed)` when
input is insufficient.
identical pattern: accumulate, attempt parse, handle partial.


the C++ implementation threads state through `this` implicitly.
the pure functional view threads it explicitly. same machine,
different notation.
