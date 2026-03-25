# error codes

mapping protocol violations to HTTP status codes.

upstream: `1_protocol.md` (fail-fast rationale, error codes overview)


---


## codes used

| code | name                            | meaning                          |
|------|---------------------------------|----------------------------------|
| 400  | Bad Request                     | malformed syntax                 |
| 413  | Content Too Large               | body exceeds size limit          |
| 414  | URI Too Long                    | request-target exceeds limit     |
| 431  | Request Header Fields Too Large | header(s) exceed size limit      |
| 501  | Not Implemented                 | method not supported             |
| 505  | HTTP Version Not Supported      | not HTTP/1.0 or HTTP/1.1         |


---


## by parse phase


### request-line
```
request-line = method SP request-target SP HTTP-version CRLF
```

| violation                        | code | rationale                          |
|----------------------------------|------|------------------------------------|
| method not GET/POST/DELETE       | 501  | method not implemented             |
| missing SP after method          | 400  | malformed syntax                   |
| request-target exceeds limit     | 414  | URI too long                       |
| missing SP after request-target  | 400  | malformed syntax                   |
| version not HTTP/1.0 or HTTP/1.1 | 505  | version not supported              |
| missing CRLF                     | 400  | malformed syntax                   |

implementation note: 414 is documented but not yet enforced.
see `3_input-bounds/size-limits.md` for planned limits.


### headers
```
header-field = field-name ":" OWS field-value OWS
headers      = *( header-field CRLF ) CRLF
```

| violation                        | code | rationale                          |
|----------------------------------|------|------------------------------------|
| missing colon                    | 400  | malformed syntax                   |
| invalid character in field-name  | 400  | malformed syntax                   |
| header line exceeds limit        | 431  | header field too large             |
| total headers exceed limit       | 431  | header fields too large            |
| header count exceeds limit       | 431  | header fields too large            |
| duplicate Content-Length         | 400  | ambiguous, potential smuggling     |
| Content-Length not a number      | 400  | malformed syntax                   |
| Content-Length negative          | 400  | invalid value                      |

implementation note: 431 is documented but not yet enforced.
see `3_input-bounds/size-limits.md` for planned limits.


### body

| violation                        | code | rationale                          |
|----------------------------------|------|------------------------------------|
| Content-Length exceeds limit     | 413  | content too large                  |
| chunk size exceeds remaining     | 413  | content too large (chunked path)   |
| malformed chunk size             | 400  | invalid hexadecimal                |
| chunk size overflow              | 400  | value exceeds representable range  |


---


## design notes


### 400 as default

400 Bad Request is the catch-all for syntax violations.
when no more specific code applies, use 400.

RFC 9110 §15.5.1: "the server cannot or will not process the request
due to something that is perceived to be a client error."


### 501 vs 405

501 Not Implemented: the server does not recognise or support the method.
405 Method Not Allowed: the method is known but not permitted for this resource.

the frontend produces 501 — it rejects unknown methods globally.
405 is a routing decision: "this resource does not accept DELETE."
that belongs downstream (Router), not to the parser.


### 431 vs 400

431 Request Header Fields Too Large is specific to header size violations.
it signals: "your headers are too big" rather than "your request is malformed."

the distinction matters for client retry behaviour. 431 tells the client
it might succeed with fewer or smaller headers. 400 gives no such hint.


### duplicate Content-Length

duplicate Content-Length headers with differing values are rejected with 400.
conflicting lengths enable request smuggling attacks.
```
Content-Length: 10
Content-Length: 20
```

some servers take the first, some take the last, some concatenate.
we reject. no ambiguity, no attack surface.

RFC 9110 §8.6: "If the message is received with a Content-Length
header field having the same value in multiple members, or having
a single value that is not a valid non-negative integer, then the
message is invalid..."


### chunked encoding

Transfer-Encoding: chunked is supported.

body accumulation is bounded by `max_body_size_`. each chunk's size
is validated: the cumulative body length must not exceed the limit.
violation produces 413.

chunk size parsing rejects:
- non-hexadecimal characters → 400
- overflow (value exceeds size_t) → 400
- missing CRLF after chunk data → 400

trailer headers after the final zero-length chunk are currently
discarded without parsing. they are not validated.


---


## limits

size limits that trigger 413, 414, 431 are defined in
`3_input-bounds/size-limits.md`.

current implementation status:

| limit                | status       | code |
|----------------------|--------------|------|
| body size            | enforced     | 413  |
| URI length           | not enforced | 414  |
| header line length   | not enforced | 431  |
| header count         | not enforced | 431  |
| total headers size   | not enforced | 431  |

the error code documents *what* is produced.
the limits document defines *when* the threshold is crossed.
