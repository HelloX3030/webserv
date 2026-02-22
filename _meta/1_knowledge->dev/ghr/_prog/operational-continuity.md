# operational continuity

## specification

server remains available under:
- stress testing
- invalid input
- client misbehavior

resilience is mandatory across all operational scenarios.

---

## failure modes and countermeasures

### client disconnection

detection:
- `recv()` returns 0 → client closed cleanly (FIN received)
- `recv()` returns -1, errno ECONNRESET → connection reset (RST received)
- `send()` returns -1, errno EPIPE or ECONNRESET → client gone

response:
close fd, free connection state, continue serving others.
no crash, no leak, no stall.

### fd exhaustion

cause: too many open connections.

countermeasure: timeout mechanism.
stale connections closed, fds recycled.
see: request-timeout.md

### memory exhaustion

cause: unbounded buffers.
client sends 10GB header. server allocates 10GB. dead.

countermeasure: size limits.
- max header size (e.g. 8KB)
- max body size (configurable, e.g. 1MB default)
- max URI length (e.g. 8KB)

on limit exceeded: return 413 (body) or 431 (headers), close connection.

### CPU starvation

cause: 1 client monopolises processing.

countermeasure: non-blocking I/O.
no single `recv()` or `send()` blocks the event loop.
all clients serviced in round-robin by event availability.

### invalid input

cause: malformed HTTP, binary garbage, protocol violations.

countermeasure: defensive parsing.
parser must not crash on any input.
invalid input → 400 Bad Request → close connection.

no assumptions about input validity.
every byte is untrusted until validated.

### client misbehavior

**slowloris**: send request byte-by-byte to hold connection indefinitely.
countermeasure: timeout. connection idle too long → closed.

**huge requests**: oversized headers/body to exhaust memory.
countermeasure: size limits (see above).

**rapid reconnect flood**: exhaust fd table with new connections.
countermeasure: timeout cleans stale connections.
optional: connection rate limiting (not required for webserv).

---

## invariant

no client action crashes the server.
no client action makes the server unresponsive to other clients.
no client action leaks resources permanently.