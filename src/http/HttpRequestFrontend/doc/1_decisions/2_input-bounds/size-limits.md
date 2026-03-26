# input bounds


## the problem

HTTP defines no upper limit on request-line or header size.
RFC 9110: "The HTTP protocol does not place any a priori limit
on the length of a URI."

without limits, the parser accumulates unboundedly.
a malicious client sends an infinitely long URI or header line,
buffer grows until memory exhaustion. DoS vector.


---


## the limits

| constant | value | scope |
|----------|-------|-------|
| `MAX_REQUEST_LINE` | 8192 | method SP uri SP version |
| `MAX_HEADER_BYTES` | 32768 | total header section |

defined in `HttpRequestFrontend_internal.hpp`.


---


## origin of values

industry convention, not RFC specification.

request-line:
  nginx default is 8192 bytes (`large_client_header_buffers`).
  Apache default is 8190 bytes (`LimitRequestLine`).
  WebServ uses 8192.

header total:
  nginx default is 4 buffers × 8KB = 32KB.
  Apache uses 8190 per field (`LimitRequestFieldSize`)
  with no explicit total.
  WebServ uses 32768 for total header bytes.

no legitimate client exceeds these. any request larger is either
broken or malicious.


---


## error codes

| condition | code | defined in |
|-----------|------|------------|
| request-line exceeds 8192 bytes | 414 URI Too Long | RFC 9110 §15.5.15 |
| header section exceeds 32768 bytes | 431 Request Header Fields Too Large | RFC 6585 §5 |

431 covers both "single header too large" and "total headers too large".
WebServ checks total bytes only — sufficient for a student project,
and simpler than tracking per-line and count separately.


---


## where checks occur

`parse_request_line()`:
- if no CRLF found and `buffer_.size() >= MAX_REQUEST_LINE + CRLF_LEN` → 414
- if CRLF found and `crlf_pos > MAX_REQUEST_LINE` → 414

`parse_header_line()`:
- track cumulative bytes consumed in header section
- if total exceeds `MAX_HEADER_BYTES` → 431


---


## design decision: hardcoded, not configurable

`max_body_size` is a constructor parameter, injected from config.
body size varies legitimately by application (file uploads vs API).

URI and header limits are security constraints, not application policy.
no legitimate reason to exceed 8KB request-line or 32KB headers.
hardcoding avoids config schema changes and removes a knob that
should never be turned.

constants live in `HttpRequestFrontend_internal.hpp` alongside
`CRLF_LEN` — protocol constants, not tunables.


---


## implementation status

| check | status |
|-------|--------|
| 414 request-line | implemented |
| 431 header total | implemented |
| 413 body (Content-Length) | implemented |
| 413 body (chunked) | implemented |
