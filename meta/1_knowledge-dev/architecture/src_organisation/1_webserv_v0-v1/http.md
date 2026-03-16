## predicate

protocol layer. HTTP syntax and semantics.

- request parsing: bytes → HttpRequest
- response building: HttpResponse → bytes
- routing: (HttpRequest, ServerConfig) → HandlerDecision

Router answers "given this request, what should happen?"
protocol-layer logic that consumes config, not config-layer logic.

---

## naming

"http" — the protocol.

---

## v0 → v1

removed: `handlers/` (method implementations) → now top-level `handlers/`

handlers use HTTP types but aren't *about* HTTP.
different concern, different rate of change.
