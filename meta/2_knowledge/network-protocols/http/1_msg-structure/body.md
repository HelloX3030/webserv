# body semantics


## delimitation vs interpretation

HTTP defines how to delimit the body: where it starts, where it ends.
HTTP does not define what the body means.

delimitation mechanisms (HTTP/1.1):
- `Content-Length`: exactly n octets after the header section
- `Transfer-Encoding: chunked`: framed chunks until zero-length terminator
- connection close: body extends until TCP close (HTTP/1.0 fallback; unreliable)

interpretation is application-level.
the protocol transports octets; meaning belongs to handlers.


---


## the grammar

RFC 9112 section 6:
```abnf
message-body = *OCTET
```

`*OCTET` — zero or more arbitrary bytes.
no structure imposed by HTTP itself.

the body is opaque at the protocol layer.


---


## content examples

the same HTTP framing carries any of:

- JSON: `{"user": "ghr", "action": "submit"}`
- form data: `name=ghr&action=submit`
- binary image: `\x89PNG\r\n\x1a\n...`
- arbitrary octets

the framing layer extracts bytes correctly.
it does not parse, validate, or transform content.


---


## where interpretation occurs

`Content-Type` header declares the media type.
handlers, CGI processes, and application logic interpret based on that declaration.

```
Content-Type: application/json     → JSON parser
Content-Type: text/html            → HTML renderer
Content-Type: image/png            → image decoder
Content-Type: application/octet-stream → raw binary
```

HTTP's responsibility ends at correct extraction.
the body field contains raw octets, nothing more.


---


## length determination precedence

RFC 9112 section 6 specifies the order:

1. `Transfer-Encoding` present and final encoding is `chunked`
   → length determined by chunk boundaries

2. `Content-Length` present (and no conflicting Transfer-Encoding)
   → body is exactly this many octets

3. neither present
   → for requests: no body (or error, depending on method)
   → for responses: body extends until connection close

multiple `Content-Length` values that differ → malformed message (400).

`Transfer-Encoding` takes precedence over `Content-Length`.
if both present, `Content-Length` is ignored.


---


## references

RFC 9112 §6 — Message Body
    https://www.rfc-editor.org/rfc/rfc9112#section-6

RFC 9110 §8.6 — Content-Length
    https://www.rfc-editor.org/rfc/rfc9110#section-8.6

RFC 9112 §7 — Transfer-Encoding
    https://www.rfc-editor.org/rfc/rfc9112#section-7
