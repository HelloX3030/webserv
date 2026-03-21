# host header enforcement

## the question

HTTP/1.1 requests without a `Host` header: accept or reject?


---


## the requirement

RFC 9112 §3.2:
a client sending an HTTP/1.1 request must include a single
`Host` header field. a server receiving a request without
`Host` must respond with 400 Bad Request.


---


## the decision

HTTP/1.1 requests with no `Host` header are rejected with 400.

HTTP/1.0: no enforcement. `Host` was not defined until HTTP/1.1;


---


## where enforcement happens

at the HEADERS → BODY transition: the empty-line detection
in `parse_header_line()`, before body length computation.

this is the earliest moment at which the complete header set
is available. enforcing earlier is impossible — headers arrive
incrementally and `Host` may appear at any position.


---


## what is checked

presence of the key `"host"` in `request_.headers`.

the value is not validated — `Host` value checking
(virtual host selection, malformed host syntax) belongs
to the routing layer, not the frontend.
