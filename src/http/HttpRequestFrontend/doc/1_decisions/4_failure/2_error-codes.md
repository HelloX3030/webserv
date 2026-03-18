# error codes

mapping protocol violations to HTTP status codes.


---


## codes used

| code | name                           | meaning                          |
|------|--------------------------------|----------------------------------|
| 400  | Bad Request                    | malformed syntax                 |
| 413  | Content Too Large              | body exceeds size limit          |
| 414  | URI Too Long                   | request-target exceeds limit     |
| 431  | Request Header Fields Too Large| header(s) exceed size limit      |
| 501  | Not Implemented                | method or feature not supported  |
| 505  | HTTP Version Not Supported     | not HTTP/1.0 or HTTP/1.1         |


---


## by parse phase


### request-line
```
request-line = method SP request-target SP HTTP-version CRLF
```

| violation                        | code | rationale                        |
|----------------------------------|------|----------------------------------|
| method not GET/POST/DELETE       | 501  | method not implemented           |
| missing SP after method          | 400  | malformed syntax                 |
| request-target exceeds limit     | 414  | URI too long                     |
| missing SP after request-target  | 400  | malformed syntax                 |
| version not HTTP/1.0 or HTTP/1.1 | 505  | version not supported            |
| missing CRLF                     | 400  | malformed syntax                 |
| request-line exceeds limit       | 400  | malformed syntax (no specific code) |


### headers
```
header-field = field-name ":" OWS field-value OWS
headers      = *( header-field CRLF ) CRLF
```

| violation                        | code | rationale                        |
|----------------------------------|------|----------------------------------|
| missing colon                    | 400  | malformed syntax                 |
| invalid character in field-name  | 400  | malformed syntax                 |
| header line exceeds limit        | 431  | header field too large           |
| total headers exceed limit       | 431  | header fields too large          |
| duplicate Content-Length         | 400  | ambiguous, potential smuggling   |
| Content-Length not a number      | 400  | malformed syntax                 |
| Content-Length negative          | 400  | invalid value                    |


### body

| violation                        | code | rationale                        |
|----------------------------------|------|----------------------------------|
| POST without Content-Length      | 400  | missing required field           |
| Content-Length exceeds limit     | 413  | content too large                |
| Transfer-Encoding: chunked       | 501  | not implemented                  |


---


## design notes


### 400 as default

400 Bad Request is the catch-all for syntax violations.
when no more specific code applies, use 400.


### 501 vs 405

501 Not Implemented: the server does not recognise or support the method.
405 Method Not Allowed: the method is known but not permitted for this resource.

the frontend produces 501 — it rejects unknown methods globally.
405 is a routing decision: "this resource does not accept DELETE."
that belongs to Router, not the parser.


### 431 vs 400

431 Request Header Fields Too Large is specific to header size violations.
it signals: "your headers are too big" rather than "your request is malformed."
the client might retry with fewer or smaller headers.


### smuggling considerations

duplicate Content-Length headers are rejected with 400.
conflicting lengths enable request smuggling attacks.
the parser must not tolerate ambiguity.
```
Content-Length: 10
Content-Length: 20
```

some servers take the first, some take the last, some concatenate.
we reject. no ambiguity, no attack surface.


---


## limits

size limits that trigger 413, 414, 431 are defined in
`1_decisions/3_input-bounds/size-limits.md`.

the error code documents *what* is produced.
the limits document defines *when* the threshold is crossed.
