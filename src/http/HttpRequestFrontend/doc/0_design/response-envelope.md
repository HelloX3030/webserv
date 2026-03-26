# response envelope

HTTP messages separate framing from payload.
the protocol specifies how to delimit and label a message,
not what the message contains.


---


## message structure

an HTTP message is an envelope carrying arbitrary octets.

```
┌─────────────────────────────────────┐
│           start-line                │  request-line or status-line
├─────────────────────────────────────┤
│           headers                   │  metadata about the message
├─────────────────────────────────────┤
│           blank line                │  CRLF — header terminator
├─────────────────────────────────────┤
│           message-body              │  payload — opaque octets
└─────────────────────────────────────┘
```

the grammar (RFC 9112) governs the envelope.
the body is `*OCTET` — no structure imposed by HTTP itself.


---


## response grammar

RFC 9112 section 4:

```abnf
HTTP-response  = status-line
                 *( field-line CRLF )
                 CRLF
                 [ message-body ]

status-line    = HTTP-version SP status-code SP [ reason-phrase ] CRLF
status-code    = 3DIGIT
reason-phrase  = 1*( HTAB / SP / VCHAR / obs-text )
```

this specifies:
- status-line format: `HTTP/1.1 404 Not Found\r\n`
- header syntax: identical to request headers
- body delimitation: Content-Length or chunked encoding

it does not specify:
- body content structure
- HTML format
- JSON schema
- any interpretation of payload bytes


---


## payload opacity

```abnf
message-body = *OCTET
```

from the HTTP layer's perspective, the body is a byte sequence.
Content-Type is metadata informing the recipient how to interpret those bytes.
the HTTP layer itself performs no body parsing or validation.

an HTML error page:

```html
<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body><h1>Not Found</h1></body>
</html>
```

is payload, not HTTP structure.
it occupies the message-body slot.
the HTTP grammar neither knows nor cares that it is HTML.


---


## layer separation

two distinct grammars:

| layer | grammar | governs |
|-------|---------|---------|
| HTTP | RFC 9112 | envelope structure |
| HTML | WHATWG HTML spec | body interpretation |

these layers are orthogonal.
an HTTP message can carry HTML, JSON, PNG, arbitrary binary.
the HTTP framing is identical regardless of payload type.

```
HTTP/1.1 404 Not Found\r\n        ← HTTP layer (RFC 9112)
Content-Type: text/html\r\n       ← HTTP layer
Content-Length: 98\r\n            ← HTTP layer
\r\n                              ← HTTP layer (header terminator)
<!DOCTYPE html>...                ← payload (WHATWG HTML)
```


---


## frontend responsibility

HttpResponseFrontend serialises the envelope, not the payload.

input: HttpResponse struct with status, headers, body bytes already populated.
output: wire-format byte sequence per RFC 9112.

```cpp
std::string HttpResponseFrontend::serialize(const HttpResponse& resp);
```

the frontend:
- emits status-line
- emits headers with proper OWS handling
- emits CRLF terminator
- emits body bytes verbatim

body content origin is not the frontend's concern.
handlers, CGI, static file serving — these populate the body.
the frontend receives bytes; it frames and transmits bytes.


---


## symmetry with request frontend

| frontend | direction | operation |
|----------|-----------|-----------|
| HttpRequestFrontend | wire → struct | parse envelope, extract body |
| HttpResponseFrontend | struct → wire | serialise envelope, emit body |

both operate on the HTTP envelope.
neither interprets body content.

the request frontend extracts body bytes based on Content-Length.
the response frontend emits body bytes and sets Content-Length.
body semantics belong to application logic, not HTTP framing.


---


## error page generation

body content for error responses originates from:

1. static files: `/error_pages/404.html` read from disk
2. handler generation: constructed HTML string
3. default fallback: hardcoded template

this is application logic.
the frontend receives a populated HttpResponse.
provenance of body bytes is opaque to the serialiser.


---


## references

RFC 9112 section 4: response
    https://www.rfc-editor.org/rfc/rfc9112#section-4

RFC 9110 section 15: status codes
    https://www.rfc-editor.org/rfc/rfc9110#section-15
