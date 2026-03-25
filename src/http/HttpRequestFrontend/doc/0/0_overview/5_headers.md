# http headers


## ontology

headers are metadata about the message.

the message has two parts: metadata and content.
- **headers**: describe properties of the message, the resource, or the connection
- **body**: the content itself (if any)

headers answer questions:
- what is this content? (`Content-Type`)
- how large is it? (`Content-Length`)
- who is sending this? (`User-Agent`, `Host`)
- how should it be cached? (`Cache-Control`)
- what encoding is acceptable? (`Accept-Encoding`)


---


## structure

each header is a key-value pair:
```
field-name ":" OWS field-value OWS
```

**field-name**: the property being specified. case-insensitive.
**field-value**: the value. interpretation depends on the field.

example:
```
Content-Type: application/json
Content-Length: 42
Host: example.com
```


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

RFC 9110 §6–10 specifies standard headers and their semantics.


---


## key headers for webserv

**Host** (RFC 9110 §7.2)
required in HTTP/1.1. identifies the target server.
enables virtual hosting: multiple domains on one IP.
```
Host: example.com
```

**Content-Length** (RFC 9110 §8.6)
body size in bytes. determines where the body ends.
```
Content-Length: 1024
```

**Transfer-Encoding** (RFC 9112 §6.1)
how the body is encoded for transfer. `chunked` means framed.
```
Transfer-Encoding: chunked
```

**Content-Type** (RFC 9110 §8.3)
media type of the body. tells handlers how to interpret content.
```
Content-Type: text/html; charset=utf-8
```

**Connection** (RFC 9110 §7.6.1)
controls connection persistence.
```
Connection: keep-alive
Connection: close
```


---


## case insensitivity

RFC 9110 §5.1: field names are case-insensitive.

`Content-Type`, `content-type`, `CONTENT-TYPE` are equivalent.

HttpRequestFrontend normalises to lowercase during parsing.
downstream code operates on the canonical form.


---


## multiple headers with same name

RFC 9110 §5.3: multiple field-lines with the same name
are equivalent to a single field-line with comma-joined values.
```
Accept: text/html
Accept: application/json
```

is semantically identical to:
```
Accept: text/html, application/json
```

HttpRequestFrontend stores the comma-joined form.

exception: `Content-Length` with differing values is malformed → 400.


---


## what HttpRequestFrontend stores
```cpp
std::map<std::string, std::string> headers;
```

- keys: lowercase field-names
- values: field-values (comma-joined if multiple)

the frontend handles:
- case normalisation
- OWS trimming
- duplicate concatenation
- Content-Length conflict detection

the frontend does not interpret header semantics beyond:
- Host presence (required for HTTP/1.1)
- Content-Length / Transfer-Encoding (body framing)
- Connection (persistence, via `keepAlive()`)


---


## downstream responsibilities

handlers interpret headers for application logic:
- Content-Type → body parsing
- Accept → content negotiation
- Authorization → access control
- Cookie → session management

the frontend extracts and normalises. handlers interpret.


---


## references

RFC 9110 §5: Fields (general structure)
RFC 9110 §6–10: specific header definitions
RFC 9112 §6: message body framing
