# persistent connections


## question

do we need to implement HTTP/1.0 keep-alive?


## answer

HTTP/1.0 keep-alive mechanism: no. not required.

HTTP/1.0 defaults to closing connections. keep-alive is an opt-in
extension — client sends `Connection: keep-alive`, server honours it.

HTTP/1.1 inverts the default: connections persist unless either
party sends `Connection: close`. the opt-in mechanism is replaced by opt-out.

since the subject mandates HTTP/1.1, we implement HTTP/1.1 semantics.
the HTTP/1.0 keep-alive handshake is unnecessary — we handle
persistence through HTTP/1.1's default behaviour.


---


## what persistence demands

implementing persistent connections correctly forces discipline
across multiple components. the requirements trace as follows.


### request boundary detection

without persistence: 1 connection = 1 request.
EOF signals end-of-request. boundary detection can be approximate.

with persistence: multiple requests flow over the same fd.
EOF no longer signals anything — the connection stays open.
the frontend must know exactly where one request ends.

this forces:
- correct Content-Length parsing and enforcement
- state machine that tracks "request complete" vs "more bytes expected"
- no reliance on connection close as implicit terminator

approximate boundary detection survives single-request conditions.
it fails when connections persist.


### mental model

without persistence: server thinks in "connections."
accept, handle, close, next.

with persistence: server thinks in "requests flowing over connections."
a connection is a substrate. requests are units of work.
the fd outlives any single request.

this is what HTTP/1.1 actually describes.
implementing persistence forces the architecture to match the
protocol's semantics rather than an approximation.


### state machine extension

after each response: explicit decision required.
keep fd open, or close?

the runtime must handle a state that doesn't exist in the
single-request model: "idle, waiting for next request."
fd registered with poll(), but neither reading nor writing.

this is where subtle bugs concentrate — state transitions that
only exist under persistence, invisible in single-request testing.


### practical effects

browsers assume persistence by default.
without support: repeated TCP handshakes, confusing debug output.

stress tests with connection reuse behave differently than
tests that open fresh connections. connection overhead matters
for the mandatory stress testing requirement.


---


## the persistence decision

after each response, the server must decide: keep connection open,
or close it?

this decision requires:

1. HTTP version (1.1 = persist by default, 1.0 = close by default)
2. Connection header value from request
3. whether response completed cleanly (I/O success)
4. server config (timeout, max requests — if implemented)
5. ability to act on the decision (keep fd registered, or close it)


### who has what
```
                        | frontend | response | runtime
------------------------+----------+----------+---------
HTTP version            |    ✓     |          |
Connection header       |    ✓     |          |
response completion     |          |    ✓     |
config                  |          |          |    ✓
fd lifecycle control    |          |          |    ✓
```

frontend has (1) and (2), but cannot have (3), (4), or (5).
response builder has (3), but cannot have (1), (2), (4), or (5).
runtime has (4) and (5), and can read (1), (2), (3) from the others.

only runtime has access to all inputs and ability to act.
therefore: runtime owns the decision.


### module contracts

HTTP request frontend:
- expose HTTP version in request struct
- expose Connection header in headers map
- expose Content-Length for body boundary detection
- provide `keepAlive()` method: pure derivation from version + header

response builder:
- expose whether response completed cleanly
- provide `completedCleanly()` method: true iff all bytes written
  and no I/O error

runtime:
- call `request.keepAlive() && response.completedCleanly()`
- if true: reset connection state, keep fd registered
- if false: close fd, deregister from poll


---


## runtime decision point
```cpp
/* after response written */
if (request.keepAlive() && response.completedCleanly())
{
    connection.resetForNextRequest();
}
else
{
    connection.close();
}
```

the conjunction matters: even if client wants keep-alive,
a failed response should close. don't reuse connections in
uncertain state.


---


## completedCleanly() definition

what does "clean completion" mean?

from first principles:
- all response bytes written to socket
- no I/O error during write


### should error responses close the connection?

intuition: "something went wrong, close the connection."

but this conflates:
1. response semantics (what the response means)
2. connection health (whether the transport is reliable)

a 404 or 500 is a valid, complete HTTP response.
the connection transported it correctly.
nothing is wrong with the connection itself.

NGINX does not close on 4xx or 5xx. closes on: idle timeout,
request count limit, network errors, client disconnect.

decision based on connection health, not response semantics.


### definition
```
completedCleanly(response) :=
    response.all_bytes_written AND NOT response.had_io_error
```

status code is irrelevant. what matters: did the bytes reach
the socket without I/O failure?


---


## open: connection timeout

if fd sits idle waiting for next request, when is it reaped?

a persistent connection without timeout is a resource leak.
malicious clients can hold connections indefinitely.

mechanism required:
- timestamp of last activity per connection
- periodic check: if (now - last_activity) > timeout → close

reference values:
- NGINX: `keepalive_timeout` default 75 seconds
- Apache: `KeepAliveTimeout` default 5 seconds

this is a runtime concern. document separately under
runtime/connection-lifecycle.