# body semantics


## delimitation vs interpretation

HTTP defines how to delimit the body: where it starts, where it ends.

- `Content-Length`: exactly n bytes after the header section
- `Transfer-Encoding: chunked`: chunk frames until last-chunk

HTTP does not define what the body means. that is application semantics.


---


## the grammar

RFC 9112 section 6:
```abnf
message-body = *OCTET
```

`*OCTET` — zero or more arbitrary bytes. no structure imposed by HTTP.


---


## examples of body content

the same HTTP framing carries any of:

- JSON: `{"user": "ghr", "action": "submit"}`
- form data: `name=ghr&action=submit`
- binary image: `\x89PNG\r\n\x1a\n...`
- arbitrary octets

the parser extracts bytes correctly. it does not understand them.


---


## where interpretation occurs

`Content-Type` header declares the media type.
handlers, CGI, application logic interpret based on that declaration.

the frontend's job ends at correct extraction.
`request_.body` contains the raw bytes. nothing more.


---


## implication for implementation

`body` is `std::string` used as a byte buffer.
no parsing, no validation, no transformation of content.
chunked decoding removes framing; the result is still raw bytes.
```
