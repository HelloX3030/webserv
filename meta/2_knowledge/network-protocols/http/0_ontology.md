# http ontology

what HTTP is at the archetypal level.
from which grammar, semantics, and implementation derive.


---


## the request-response pattern

HTTP instantiates a universal pattern: request-response.
```
Initiator  ────request────▶  Responder
           ◀───response────
```

this pattern appears across domains:
- function call: arguments → return value
- query: question → answer
- command: imperative → acknowledgement

the pattern has invariant structure:
1. initiator formulates a request (intent + parameters)
2. responder receives, interprets, acts
3. responder formulates a response (outcome + data)
4. initiator receives, interprets

HTTP is this pattern specialised for hypermedia resource access
over an internetwork.


---


## statelessness

each request-response exchange is semantically independent.
the server retains no memory of prior requests.

this is a design constraint.
consequences:

**horizontal scalability** — any server instance can handle any request.
no session affinity required. load balancing is trivial.

**simplicity** — no session state machine on the server.
each request is self-contained, carrying all necessary context.

**reliability** — no shared state to corrupt or lose.
server restart does not invalidate client sessions (there are none).

**explicit state mechanisms** — applications requiring state must
layer it explicitly: cookies, tokens, session identifiers.
state becomes visible, auditable, controllable.

statelessness is HTTP's fundamental constraint.
violating it (server-side sessions without explicit tokens)
fights the protocol's architecture.


---


## resource-orientation

HTTP operates on resources.

a resource is any addressable entity: a document, an image, a service,
an abstract concept, a computation. resources are identified by URIs.

```
URI  →  resource  →  representation
```

    the URI identifies the resource.
    the resource is the abstract entity.
    the representation is a concrete rendering (HTML, JSON, image bytes).


methods are operations on resources:
- GET: retrieve a representation
- POST: submit data for processing by the resource
- PUT: replace the resource with the payload
- DELETE: remove the resource

this is the REST insight made explicit: HTTP is an interface
to a resource space, not a procedure call mechanism.
the uniform interface (methods × URIs) replaces ad-hoc APIs.


---


## layering

HTTP occupies a specific position in the protocol stack:
```
┌─────────────────────────────────────┐
│  application data (HTML, JSON...)   │  ← representation format
├─────────────────────────────────────┤
│              HTTP                   │  ← message framing, semantics
├─────────────────────────────────────┤
│         TLS (optional)              │  ← encryption, authentication
├─────────────────────────────────────┤
│              TCP                    │  ← reliable ordered byte stream
├─────────────────────────────────────┤
│              IP                     │  ← addressing, routing
├─────────────────────────────────────┤
│           link layer                │  ← physical transmission
└─────────────────────────────────────┘
```

HTTP assumes TCP semantics: bytes arrive in order, none lost.
but arrival is chunked arbitrarily by the network.
this creates the streaming parser problem — the reason
HttpRequestFrontend exists as a stateful machine.

HTTP defines message structure and method semantics.
it does not define representation format (that is application-level)
nor transport reliability (that is TCP's concern)
nor security (that is TLS's concern).

each layer has a single responsibility. HTTP's responsibility:
structure requests and responses for resource operations.


---


## what HTTP is not

**not a transport protocol** — TCP provides reliable delivery.
HTTP consumes that service; it does not provide it.

**not a representation format** — HTML, JSON, XML are payloads.
HTTP carries them; it does not define their structure.

**not inherently secure** — HTTP transmits in cleartext.
security requires TLS layered below (HTTPS).

**not stateful** — applications requiring state must build it.
HTTP provides the mechanism (cookies, headers) but not the state.

**not a programming API** — HTTP is a wire protocol.
libraries (libcurl, requests, fetch) expose it as an API.
the protocol itself is byte sequences on a socket.


---


## the wire format (HTTP/1.x)

HTTP/1.x is textual: human-readable ASCII with defined delimiters.
```
start-line CRLF
*( header-field CRLF )
CRLF
[ message-body ]
```

this structure is invariant for both requests and responses.
only the start-line differs:
- request: method SP request-target SP HTTP-version
- response: HTTP-version SP status-code SP reason-phrase

the textual format enables:
- debugging with simple tools (telnet, nc, curl -v)
- gradual learning (read raw messages, understand protocol)
- extensibility (new headers are just new text lines)

HTTP/2 and HTTP/3 abandon text for binary framing.
the semantics remain; the wire format changes.
understanding HTTP/1.x wire format is foundational.


---


## references

RFC 9110: HTTP Semantics
    https://www.rfc-editor.org/rfc/rfc9110
    the authoritative source for what HTTP means.

RFC 9112: HTTP/1.1
    https://www.rfc-editor.org/rfc/rfc9112
    the wire format specification.

Fielding, R. T. (2000). Architectural Styles and the Design of
Network-based Software Architectures. Doctoral dissertation.
    https://www.ics.uci.edu/~fielding/pubs/dissertation/top.htm
    the REST dissertation. chapter 5 defines the architectural style.
    HTTP is designed to embody these constraints.
