# failure categories

three categories of failure, distinguished by cause and response.


---


## protocol failure

input violates HTTP grammar or semantic constraints.

cause: the client sent bytes that do not constitute a valid request.
examples:
- malformed request-line (missing SP, invalid method)
- header syntax violation (missing colon, illegal characters)
- Content-Length absent for POST
- Content-Length exceeds limit
- unsupported HTTP version
- unsupported transfer encoding

response: return HTTP error code.
the frontend produces a code; the runtime sends the response.

this is the frontend's primary failure mode.
the frontend is a pure transformation: bytes → HttpRequest ∪ ErrorCode.
all protocol failures produce error codes, not exceptions.


---


## system failure

an OS-level operation fails.

cause: the environment cannot satisfy a request.
examples:
- read() returns -1 (socket closed, timeout)
- write() fails (peer disconnected)
- resource exhaustion (fd limit, memory)

response: handled by runtime, not frontend.

the frontend never performs I/O. it receives bytes already read.
system failures are invisible to it — they occur in Connection,
which owns the socket and calls read().

when Connection detects a system failure mid-parse:
- log the event
- close the connection
- destroy the frontend instance

no HTTP response is possible if the socket is dead.


---


## programming error

a bug in the code.

cause: violated invariant, logic error, undefined behaviour.
examples:
- null pointer dereference
- buffer overrun
- impossible state reached

response: not "handled" at runtime.

programming errors indicate a defect. the correct response is:
- in debug builds: assert and abort, produce diagnostic
- in release builds: undefined behaviour (if not caught) or crash

the frontend uses assertions for internal invariants:
```cpp
assert(phase_ != ParsePhase::Complete);  // advance() precondition
```

if an assertion fires, the program is wrong. no recovery.


---


## the boundaries
```
┌─────────────────────────────────────────────────────────────────┐
│                         RUNTIME (Connection)                    │
│                                                                 │
│   ┌─────────────┐     ┌─────────────────────────────────────┐   │
│   │   read()    │────▶│       HttpRequestFrontend           │   │
│   │  (system)   │     │                                     │   │
│   └─────────────┘     │  protocol failure → ErrorCode       │   │
│         │             │  programming error → assert         │   │
│         │             └─────────────────────────────────────┘   │
│         │                              │                        │
│         ▼                              ▼                        │
│   system failure              protocol failure                  │
│   → close connection          → send error response             │
│   → log                       → close or keep-alive             │
│                               → log                             │
└─────────────────────────────────────────────────────────────────┘
```

the frontend's contract: given valid bytes, produce result.
system failures are upstream (before bytes reach frontend).
programming errors are prevented by correct code, caught by assertions.
protocol failures are the expected failure mode — the frontend is *designed* to detect them.


---


## summary

| category    | cause              | detected by      | response           |
|-------------|--------------------|-----------------|--------------------|
| protocol    | invalid input      | frontend        | error code         |
| system      | OS failure         | runtime         | close, log         |
| programming | bug                | assertion       | abort              |

protocol failures are normal operation.
system failures are environmental.
programming errors are defects.
