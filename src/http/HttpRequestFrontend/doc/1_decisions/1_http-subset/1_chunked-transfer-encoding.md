# chunked transfer encoding

## the question

must the server handle `Transfer-Encoding: chunked`?


---


## the requirement

42 subject (body handling):
> chunked transfer encoding: server un-chunks before passing to CGI

42 subject (CGI communication):
> for chunked requests, your server needs to un-chunk them;
the CGI will expect EOF as the end of the body


chunked un-chunking is mandatory.


---


## the decision

chunked transfer encoding is implemented in `consume_body()`.

the frontend decodes chunked bodies before they reach any consumer.
`HttpRequest.body` always contains plain decoded bytes,
regardless of wire encoding.


---


## why the frontend owns this

the frontend's contract: wire bytes → `HttpRequest`.
chunked encoding is wire format — a framing mechanism,
not a semantic property of the body.

the alternative — passing raw chunked bytes to the CGI layer
for un-chunking there — fractures the body contract:
some consumers would receive encoded bytes, some decoded.
`max_body_size_` enforcement would be ambiguous
(encoded size vs decoded size).

un-chunking in the frontend is the only coherent position.


---


## wire format (RFC 9112 §7.1)
```abnf
chunked-body = *chunk last-chunk CRLF
chunk        = chunk-size CRLF chunk-data CRLF
chunk-size   = 1*HEXDIG
last-chunk   = "0" CRLF
chunk-data   = 1*OCTET
```

each chunk: a hex size line, that many bytes, then CRLF.
a zero-size chunk signals end of body.
chunk extensions and trailers: not implemented (not required).


---


## implementation

`consume_body()` branches on `body_chunked_`.

chunked path uses a `ChunkPhase` sub-state (SIZE / DATA),
alternating between reading a chunk-size line and consuming
chunk bytes. decoded bytes are appended to `request_.body`.

`max_body_size_` is enforced against the *decoded* size,
accumulated as chunks arrive.

413 cannot be checked at the HEADERS→BODY transition
(decoded size is unknown then); it is checked per-chunk
as `request_.body` grows.
