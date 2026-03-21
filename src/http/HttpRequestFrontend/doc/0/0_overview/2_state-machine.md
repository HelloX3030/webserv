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

see `inc/http/HttpRequestFrontend.hpp` for struct fields.
see `HttpRequestFrontend_internal.hpp` for `PhaseResult`, `ChunkPhase`.

the struct holds:

```
buffer          accumulated unparsed bytes
phase           current phase
request         being built incrementally
body_remaining  bytes still expected (Content-Length path)
error_code      set on ERROR transition
max_body_size   from config, for 413 detection
body_chunked    Transfer-Encoding: chunked present
chunk_remaining bytes left in current chunk (DATA sub-phase)
chunk_phase     sub-state within chunked BODY (SIZE / DATA)
```

`buffer` accumulates bytes across `advance()` calls. consumed bytes
are erased after each successful phase transition.

`request` fields are populated incrementally: method/uri/version
after REQUEST_LINE completes, headers after each header line,
body after BODY completes.

`max_body_size` is set at construction from server configuration.
for Content-Length bodies: checked at HEADERS → BODY transition
(fail-fast: reject before allocating).
for chunked bodies: checked per-chunk as decoded bytes accumulate
(total decoded size unknown at transition).

`body_chunked` determines the BODY phase path.
`chunk_phase` and `chunk_remaining` are the sub-state machine
within the chunked path: SIZE reads a hex size line,
DATA consumes that many bytes plus trailing CRLF.


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
                    │       BODY        │  ← skip if no body
                    └─────────┬─────────┘
                              │ all body bytes consumed
                              ▼
                    ┌───────────────────┐
                    │     COMPLETE      │
                    └───────────────────┘
```

BODY skip condition: Content-Length absent or 0, and not chunked.

BODY has 2 internal paths (not separate phases):
- Content-Length: consume exactly `body_remaining` bytes.
- chunked: sub-state machine (SIZE / DATA) decoding frames
  until zero-size chunk + trailing CRLF.

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
    populate request method, uri, http_version.
    erase consumed bytes from buffer.
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
    headers complete. determine body encoding:

    chunked path (Transfer-Encoding: chunked present):
        set body_chunked = true, chunk_phase = SIZE,
        chunk_remaining = 0.
        transition → BODY.
        note: 413 cannot be checked here — decoded size unknown
        until chunks accumulate. checked per-chunk in BODY.

    Content-Length path:
        extract Content-Length from headers (0 if absent).
        if Content-Length > max_body_size: error 413 → ERROR.
        set body_remaining = Content-Length.
        if body_remaining == 0: transition → COMPLETE.
        else: transition → BODY.

if line is non-empty:
    find colon. if absent, error 400.
    extract name (before colon), value (after colon, trimmed).
    normalise name to lowercase.
    insert into request headers.
    erase consumed bytes from buffer.
    remain in HEADERS.


---


## phase: BODY

branches on `body_chunked`.

### Content-Length path

waiting for: `body_remaining` bytes in buffer.

sufficient bytes → extract, erase, transition → COMPLETE.
insufficient → NeedMore.

note: buffer may contain more than body_remaining if the client
pipelined requests. extract exactly body_remaining, leaving the
rest for the next request after reset().


### chunked path

sub-state machine alternating SIZE and DATA.

chunk_phase == SIZE:
    scan buffer for CRLF. if not found, return NeedMore.
    extract line, parse as hex integer → chunk_size.
    if chunk_size == 0:
        last-chunk. verify trailing CRLF is also present
        (buffer must contain size line + CRLF + trailer CRLF).
        if not: return NeedMore.
        consume both lines atomically. transition → COMPLETE.
    if decoded accumulation + chunk_size > max_body_size:
        error 413 → ERROR.
    consume size line. set chunk_remaining = chunk_size.
    chunk_phase → DATA.

chunk_phase == DATA:
    waiting for: chunk_remaining + CRLF bytes in buffer.
    if insufficient: return NeedMore.
    validate trailing CRLF after chunk data. if absent: 400 → ERROR.
    append chunk_remaining bytes to request body.
    consume chunk_remaining + CRLF bytes from buffer.
    chunk_phase → SIZE.

returns Advanced after each sub-state transition.
the while loop in advance() re-enters consume_body(),
processing all available chunks within a single advance() call.


---


## phase: COMPLETE

terminal. request is valid. advance() returns Complete.


---


## phase: ERROR

terminal. error_code is set. advance() returns Failed.


---


## advance() control flow

see `HttpRequestFrontend.cpp` for implementation.

phase parsers return a 3-valued result:
- Advanced: phase completed, transitioned to next phase.
- NeedMore: insufficient bytes, remain in current phase.
- Failed: parse error, transitioned to ERROR.

```
advance(data, len):
    append data to buffer

    loop:
        match phase:
            REQUEST_LINE → call parse_request_line()
            HEADERS      → call parse_header_line()
            BODY         → call consume_body()
            COMPLETE     → return Complete with request
            ERROR        → return Failed with error_code

        if NeedMore → return Incomplete
        if Failed   → return Failed with error_code
        if Advanced → continue loop
```

the loop handles the case where a single advance() call provides
enough bytes to complete multiple phases. it runs until NeedMore,
Complete, or Failed.

for chunked bodies: consume_body() returns Advanced after each
sub-state transition (SIZE → DATA, DATA → SIZE, or last-chunk
→ COMPLETE). the loop re-enters consume_body(), processing all
available chunks within a single advance() call without returning
Incomplete between chunks.


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
| Content-Length exceeds max_body_size | 413 | HEADERS |
| invalid chunk-size (non-hex or overflow) | 400 | BODY |
| decoded body exceeds max_body_size | 413 | BODY |


---


## reset()

see `HttpRequestFrontend.cpp` for implementation.

called by Connection after response sent, iff keepAlive() is true.

```
reset():
    preserve buffer (may contain pipelined bytes)
    phase          ← REQUEST_LINE
    request        ← empty
    body_remaining ← 0
    error_code     ← 0
    body_chunked   ← false
    chunk_remaining ← 0
    chunk_phase    ← SIZE
    max_body_size unchanged
```

buffer is not cleared. pipelined requests leave trailing bytes
after the current request's body. these bytes belong to the next
request and must survive the reset.

max_body_size is not reset — it derives from server configuration
and remains constant for the connection's lifetime.

chunked fields are reset defensively: they are always set fresh at
HEADERS → BODY transition, but reset ensures no ghost state from
a previous chunked request survives into a subsequent parse.


---


## invariants

1. buffer contains all unparsed bytes. consumed bytes are erased.

2. phase reflects current parse position.
    only advances, never retreats (except reset()).

3. request fields are valid for completed phases only.
   method/uri/version valid after REQUEST_LINE.
   headers valid after HEADERS.
   body valid after BODY (or empty if no body).

4. body_remaining: for Content-Length bodies, computed at
   HEADERS → BODY transition, 0 when BODY → COMPLETE.
   for chunked bodies, unused (chunk_remaining tracks per-chunk).

5. error_code is 0 unless phase is ERROR.

6. max_body_size is constant after construction.

7. body_chunked is set at HEADERS → BODY transition.
   determines which path consume_body() takes.

8. chunk_phase alternates SIZE → DATA → SIZE within
   the chunked BODY path. chunk_remaining is valid
   only during DATA and counts down to 0.


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

```
"POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhel"
                    ← headers + chunk size + partial chunk data
"lo\r\n3\r\nwo"    ← chunk CRLF + next chunk size + partial data
"rld\r\n0\r\n\r\n" ← completes chunk + last-chunk + trailer CRLF
```

the state machine handles all cases identically: accumulate into
buffer, scan for phase completion, consume and transition or
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
