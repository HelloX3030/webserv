# http/

## what this directory owns

Http representation - HTTP's syntax and semantics:

### Request parsing

(bytes → HttpRequest)

### Response building
(HttpResponse → bytes)

### Routing

ghr's Router signature: (HttpRequest, ServerConfig) → HandlerDecision

This operates on requests. It's HTTP-layer logic that consumes config, not config-layer logic.

Router answers "given this request, what should happen?"
That's closer to HTTP than to config parsing, so placed in http/


## removed from v0

Lukas' http/handlers/
which are method implementations
