# method subset

## the question

which HTTP methods does the server accept?


---


## the requirement

42 subject §5: GET, POST, DELETE are required.
no others are mentioned.


---


## the decision

accept: GET, POST, DELETE.
any other non-empty token: 501 Not Implemented.


---


## why 501, not 400

400 Bad Request means the request is malformed — the message
itself is structurally invalid.

an unknown method is structurally valid: it is a well-formed
token in the correct position. the server simply does not
implement it. RFC 9110 §15.6.2 defines 501 for exactly this:
the server does not support the functionality required to
fulfil the request.

the distinction matters: 400 signals a broken client;
501 signals a capable client requesting something the server
does not offer. different semantics, different error.


---


## method validation placement

validated in `parse_request_line()`, immediately after token
extraction. this is the earliest possible moment — method
is the first token on the wire. fail-fast: no further parsing
of a request the server will not handle.


---


## note on method semantics

method semantics (idempotency, safety, body expectations)
are not enforced at the frontend. the frontend validates
the token; routing and execution handle semantics.

RFC 9110 §9.3.1: GET should have no request body.
RFC 9110 §9.3.5: DELETE may have a body but servers may reject it.

webserv does not enforce these at parse time. a GET with
a body is parsed normally and handed to the executor.
