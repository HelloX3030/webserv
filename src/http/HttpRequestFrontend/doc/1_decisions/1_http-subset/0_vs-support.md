## version support: HTTP/1.0 and HTTP/1.1

### the question

which HTTP versions to accept?

### the analysis

HTTP/1.0 (RFC 1945): simple, no mandatory Host header, non-persistent by default.

HTTP/1.1 (RFC 9112): Host header required, persistent by default,
chunked transfer encoding defined.

HTTP/2 and HTTP/3: binary protocols, fundamentally different framing.
not relevant to this text-based parser.

webserv's scope is HTTP/1.x. both versions use the same message syntax.
differences are semantic (persistence defaults, required headers).

### the decision

accept HTTP/1.0 and HTTP/1.1.
reject others with 505 HTTP Version Not Supported.

version affects:
- `keepAlive()` default: 1.1 persistent, 1.0 not
- Host header: required for 1.1, optional for 1.0

### the principle

implement the versions the deployment context requires.
HTTP/1.x is the text protocol. later versions are different protocols.
