# http message structure

the invariant structure of HTTP/1.x messages.
requests and responses share the same form.


---


## the invariant

every HTTP/1.x message has this structure:

```
start-line CRLF
*( header-field CRLF )
CRLF
[ message-body ]
```

4 components:
1. start-line — identifies the message type and intent
2. header fields — metadata as name-value pairs
3. empty line — CRLF with no preceding content; signals headers complete
4. message body — optional payload

this structure is identical for requests and responses.
only the start-line differs.


---


## start-line

### request-line (for requests)

```
method SP request-target SP HTTP-version CRLF
```

example:
```
GET /index.html HTTP/1.1
```

components:
- `method`: the operation (GET, POST, DELETE, ...)
- `request-target`: the resource identifier (typically URI path)
- `HTTP-version`: protocol version (HTTP/1.0 or HTTP/1.1)

### status-line (for responses)

```
HTTP-version SP status-code SP reason-phrase CRLF
```

example:
```
HTTP/1.1 200 OK
```

components:
- `HTTP-version`: protocol version
- `status-code`: 3-digit result code
- `reason-phrase`: human-readable status (informational only)


---


## header fields

zero or more header lines, each:

```
field-name ":" OWS field-value OWS CRLF
```

example:
```
Content-Type: text/html; charset=utf-8
Content-Length: 1234
Host: example.com
```

`OWS` is optional whitespace (SP or HTAB).
field names are case-insensitive; values preserve case.

headers provide metadata about the message:
- content description (Content-Type, Content-Length, Content-Encoding)
- routing (Host)
- connection management (Connection, Keep-Alive)
- caching (Cache-Control, ETag, Last-Modified)
- authentication (Authorization, WWW-Authenticate)
- custom application data (X-* headers, though this convention is deprecated)

order of headers is generally not significant,
except for headers with the same name (which form a comma-separated list).


---


## empty line

a CRLF immediately following the last header-field CRLF.
this is the signal that headers are complete.

```
Host: example.com CRLF
Content-Length: 5 CRLF
CRLF                      ← empty line: headers end here
hello                     ← body begins
```

the empty line is mandatory even if no headers are present.
minimum valid request:
```
GET / HTTP/1.1 CRLF
CRLF
```


---


## message body

an optional sequence of octets.
interpretation depends on headers.

### presence

for requests:
- GET, HEAD, DELETE: typically no body
- POST, PUT: typically has body

for responses:
- 1xx, 204, 304: must not have body
- HEAD responses: no body (but Content-Length reflects what GET would return)
- all others: may have body

### length determination (HTTP/1.1)

in order of precedence:

1. **Transfer-Encoding: chunked** — body is chunked-encoded.
   length determined by chunk boundaries.

2. **Content-Length** — body is exactly this many octets.

3. **connection close** — body extends until connection closes.
   (HTTP/1.0 fallback; unreliable)

webserv implements Content-Length only.
chunked encoding is not supported.

### content

the body is opaque to HTTP.
Content-Type indicates interpretation:
- `text/html`: HTML document
- `application/json`: JSON data
- `image/png`: PNG image
- `application/octet-stream`: arbitrary binary

HTTP transports bytes. meaning is application-level.


---


## CRLF: the delimiter

HTTP/1.x uses CRLF (carriage return + line feed, `\r\n`, `%x0D %x0A`)
as the line terminator.

this is the internet standard newline, inherited from Telnet.
bare LF (`\n`) is technically non-conformant.
many implementations tolerate bare LF; strict implementations reject it.

webserv is strict: bare LF produces 400 Bad Request.


---


## example: complete request

```
POST /upload HTTP/1.1
Host: example.com
Content-Type: application/json
Content-Length: 27

{"filename": "test.txt"}
```

parsed:
- start-line: `POST /upload HTTP/1.1`
- headers: Host, Content-Type, Content-Length
- empty line: (between headers and body)
- body: `{"filename": "test.txt"}` (27 bytes)


---


## example: complete response

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 13

Hello, world!
```

parsed:
- start-line: `HTTP/1.1 200 OK`
- headers: Content-Type, Content-Length
- empty line: (between headers and body)
- body: `Hello, world!` (13 bytes)


---


## why the structure matters

the structure enables incremental parsing.

a parser can process bytes as they arrive:
1. scan for CRLF → extract start-line
2. scan for CRLF → extract each header
3. detect empty line (CRLF at buffer start) → headers complete
4. compute body length from headers
5. consume exactly that many bytes → body complete

no lookahead beyond CRLF detection.
no backtracking required.
O(n) in input length, O(1) auxiliary space.

the structure is regular (Chomsky type 3).
the only context-sensitive aspect — Content-Length determining
body length — is semantic, not syntactic.


---


## HTTP/2 and HTTP/3

HTTP/2 and HTTP/3 abandon the textual wire format.
messages are encoded as binary frames.

```
HTTP/1.1:  textual, CRLF-delimited, human-readable
HTTP/2:    binary frames over TCP
HTTP/3:    binary frames over QUIC (UDP)
```

the semantics remain: method, headers, body.
the framing changes: length-prefixed binary vs delimiter-scanned text.

understanding HTTP/1.1 message structure is foundational.
the concepts (start-line, headers, body) persist across versions.


---


## references

RFC 9112: HTTP/1.1 — section 2 (Message)
    https://www.rfc-editor.org/rfc/rfc9112#section-2

RFC 9110: HTTP Semantics — section 6 (Message Abstraction)
    https://www.rfc-editor.org/rfc/rfc9110#section-6
