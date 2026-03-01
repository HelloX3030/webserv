# persistent connections


## question

do we need to implement HTTP/1.0 keep-alive?


## answer

HTTP/1.0 keep-alive: no. not required, not worth implementing.
HTTP/1.1 makes persistent connections the default — different
mechanism, supersedes HTTP/1.0 keep-alive entirely.

HTTP/1.1 persistent connections: not explicitly required by the
42 subject, but worth implementing.

the subject mandates HTTP/1.1 compliance. RFC 7230 section 6.3:
persistent connections are the default behaviour for HTTP/1.1.
a server that closes every connection immediately is technically
compliant (client must handle this), but deviates from expected
behaviour.

mandatory subject requirements: correct parsing, correct responses,
non-blocking I/O, CGI, static serving, uploads. persistence is not
listed. but implementing it correctly is a strong test of whether
parsing and state management are genuinely solid.


---


## why implement anyway

persistent connections are valuable not for the feature itself,
but for the architectural discipline they impose on every other
component.


### request boundary detection

without persistent connections: each connection is 1 request-response
cycle. connection closes, fd dies, state cleaned up trivially.
the HTTP request frontend can rely on EOF to signal end-of-request.
boundary detection can be approximate.

with persistent connections: multiple requests flow over the same fd.
the frontend must know exactly where one request ends and the next
begins — EOF no longer helps. this forces:

- correct Content-Length parsing and enforcement
- correct chunked transfer decoding (if implemented)
- a state machine that explicitly tracks "request complete" vs
  "more bytes expected"

approximate boundary detection survives single-request conditions.
it fails when connections persist.


### mental model shift

without persistent connections: the server thinks in terms of
"connections" — accept, handle, close, next.

with persistent connections: the server must think in terms of
"requests flowing over connections". a connection is a substrate;
requests are the units of work. the fd outlives any single request.

this mental model is what HTTP/1.1 actually describes.
implementing it forces the architecture to match the protocol's
semantics rather than an oversimplified approximation.


### connection state machine

after each response: explicit decision required.
keep fd open, or close?

the runtime must handle a state that doesn't exist in the
single-request model: "connection idle, waiting for next request".
the fd is registered with poll(), but neither reading nor writing.

This could be where subtle bugs concentrate (due to state machine complexity)


### browser behaviour

browsers send requests over persistent connections by default.
they pipeline requests (send multiple before receiving responses).
without support: repeated TCP handshakes during testing,
confusing debug output when browsers assume persistence.


### stress test performance

connection reuse eliminates TCP handshake overhead.
a stress test that creates 1000 connections per second
behaves very differently than one that reuses 10 connections.
relevant for the mandatory stress testing requirement.


---


## ownership: where does the decision live?


### what the decision requires

to decide keep-alive or close, we need:

1. HTTP version of request (1.1 = persistent default, 1.0 = not)
2. Connection header value from request
3. whether response completed cleanly
4. server config (max requests, timeout — if implemented)
5. ability to act: keep fd registered, or close it


### HTTP request frontend — not good idea

the frontend has (1) and (2).
cannot have (3) or (4).
has no business touching fd lifecycle.

the frontend is a pure function: bytes in, structured request out.
giving it keep-alive responsibility corrupts its nature.


### shared connection object — not good idea

a connection object could hold: fd, request, response, state.
keep-alive decision computed from headers + response outcome.
runtime reads a flag and acts.

problem: decision is implicit, assembled from multiple places.
harder to reason about, harder to debug.
distributed logic, distributed failure modes.


### runtime — probably best

runtime owns:
- fd lifecycle
- connection state
- what happens after response is written

runtime can inspect:
- parsed request headers
- response status
- config

runtime is the only component that can act on the decision
(keep fd open vs close and deregister).

the decision is a transition in the connection state machine.
the runtime manages the state machine.
therefore: runtime owns the decision.


### module responsibilities

HTTP request frontend: expose `Connection` header and HTTP version
in request struct.

response builder: expose whether response completed cleanly.

runtime: reads both, computes decision, acts.

each module retains purity.
decision lives where consequences are executed.


---


## runtime decision point

```cpp
/* after response written */
if (request.keepAlive() && response.completedCleanly())
{
    /* reset connection state, keep fd registered */
    connection.resetForNextRequest();
}
else
{
    /* close fd, deregister from poll */
    connection.close();
}
```

the conjunction matters: even if client wants keep-alive,
a failed response should close.
don't reuse connections in uncertain state.


---


## open questions


### response.completedCleanly() — what conditions?

need to define what "clean completion" means.

from first principles:
- all response bytes written to socket
- no I/O error during write
- response is semantically complete (headers + body match
  Content-Length, or chunked termination sent)


#### should error responses close the connection?

a natural question: if the server returns 4xx or 5xx, should
it close the connection anyway, regardless of keep-alive?

intuition might suggest: "something went wrong, don't reuse
this connection." but this conflates two things:

1. response semantics (what the response means)
2. connection health (whether the transport is reliable)

a 404 or 500 is a valid, complete HTTP response. the connection
transported it correctly. nothing is wrong with the connection.

NGINX behaviour (from research):
- does not close connections based on status code alone
- uses `keepalive_timeout` (default 75s) and `keepalive_requests`
  to manage lifecycle
- closes on: idle timeout, request count limit, network errors,
  client-initiated close

NGINX does not close on 5xx specifically. the decision is based
on connection health, not response semantics.


#### definition for webserv
```
function completedCleanly(response):
    return response.all_bytes_written
       and not response.had_io_error
```

status code is irrelevant. what matters: did the bytes reach
the socket without I/O failure?

this follows NGINX precedent and aligns with protocol semantics.


### connection timeout

if fd sits idle too long waiting for next request, when is it
reaped?


#### first principles

a persistent connection without timeout is a resource leak vector.
malicious or buggy clients could hold connections open indefinitely,
exhausting server fd capacity.

the timeout mechanism requires:
- timestamp of last activity per connection
- periodic check (each poll cycle, or on connection access)
- if (now - last_activity) > timeout: close connection


#### reference implementations

NGINX:
- `keepalive_timeout` directive (default 75 seconds)
- `keepalive_requests` directive (default 1000 requests per
  connection)
- both configurable per-server

Apache:
- `KeepAliveTimeout` directive (default 5 seconds)
- `MaxKeepAliveRequests` directive (default 100)


#### relevance

timeout is part of the persistent connection implementation,
but it is a runtime mechanism, not part of the keep-alive
decision logic documented here.

should be documented separately under runtime/connection-lifecycle
or similar.

for now: note that timeout handling is required for persistent
connections to be safe. implementation details belong elsewhere.