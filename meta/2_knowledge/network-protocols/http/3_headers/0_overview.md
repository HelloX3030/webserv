# header semantics


## ontology

headers are metadata about the message.

a message has 2 parts: metadata and content.
- headers: properties of the message, the resource, or the connection
- body: the content itself (if any)

headers answer questions:
- what is this content? (`Content-Type`)
- how large is it? (`Content-Length`)
- who is sending this? (`User-Agent`, `Host`)
- how should it be cached? (`Cache-Control`)
- what encodings are acceptable? (`Accept-Encoding`)


---


## structure

each header is a key-value pair:

```abnf
header-field   = field-name ":" OWS field-value OWS
field-name     = token
field-value    = *( field-content / obs-fold )
OWS            = *( SP / HTAB )
```

**field-name**: the property being specified. case-insensitive.
**field-value**: the value. interpretation depends on the field.

example:
```
Content-Type: application/json
Content-Length: 42
Host: example.com
```

whitespace (OWS) around the value is permitted and must be trimmed.


---


## categories

headers group by concern:

| category       | examples                        | describes                        |
|----------------|---------------------------------|----------------------------------|
| request        | Host, User-Agent, Accept        | what the client wants            |
| representation | Content-Type, Content-Length    | the body's nature                |
| control        | Cache-Control, Connection       | how to handle the message        |
| authentication | Authorization, WWW-Authenticate | identity and access              |
| conditional    | If-Modified-Since, ETag         | conditional request semantics    |

RFC 9110 sections 6–10 specify standard headers and their semantics.


---


## case insensitivity

RFC 9110 §5.1: field names are case-insensitive.

`Content-Type`, `content-type`, `CONTENT-TYPE` are equivalent.

implementations typically normalise to a canonical form (usually lowercase)
during parsing. downstream code operates on the normalised form.


---


## multiple headers with same name

RFC 9110 §5.3: multiple field-lines with the same name
are semantically equivalent to a single field-line with comma-joined values.

```
Accept: text/html
Accept: application/json
```

is identical to:

```
Accept: text/html, application/json
```

this equivalence applies to headers whose values are defined as lists.
for headers where comma has different meaning, multiple field-lines
with the same name may be malformed.

exception: `Set-Cookie` cannot be combined (historical reasons).

`Content-Length` with differing values across multiple field-lines
is a malformed message → 400.


---


## key headers

**Host** (RFC 9110 §7.2)
required in HTTP/1.1. identifies the target server.
enables virtual hosting: multiple domains on one IP address.

**Content-Length** (RFC 9110 §8.6)
body size in octets. determines where the body ends.
absence with no Transfer-Encoding implies no body (for requests).

**Transfer-Encoding** (RFC 9112 §6.1)
how the body is encoded for transfer.
`chunked` indicates framed transfer; length determined by chunk boundaries.

**Content-Type** (RFC 9110 §8.3)
media type of the body. tells recipients how to interpret content.
includes optional parameters: `text/html; charset=utf-8`.

**Connection** (RFC 9110 §7.6.1)
controls connection persistence and hop-by-hop header handling.
`close` signals the connection will terminate after this message.
`keep-alive` (HTTP/1.0) requests persistent connection.


---


## references

RFC 9110 §5 — Fields
    https://www.rfc-editor.org/rfc/rfc9110#section-5

RFC 9110 §6–10 — Header Field Definitions
    https://www.rfc-editor.org/rfc/rfc9110#section-6

RFC 9112 §5 — Field Syntax
    https://www.rfc-editor.org/rfc/rfc9112#section-5
