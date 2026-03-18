# failure strategy

how each failure category is handled.


---


## protocol failures — error codes

protocol violations produce HTTP status codes as return values.
```
advance : Self × Bytes → Self × ParseResult

data ParseResult = Complete HttpRequest
                 | Incomplete
                 | Failed ErrorCode
```

the caller inspects the result. on Failed, the error code is ready
for the response. no exception unwinding. no out-parameters.

why return values, not exceptions:

1. protocol failures are expected. malformed requests are normal
   operation — clients are untrusted, networks are noisy.
   exceptions signal exceptional conditions; this is routine.

2. the error code *is* the response. HTTP defines status codes
   for parse failures: 400, 413, 501, 505. the frontend produces
   exactly what the response needs. no translation layer.

3. control flow is local. the caller decides what to do.
   log, send response, close connection — all visible at the call site.


---


## protocol failures — fail-fast

first violation terminates parsing. no recovery. no error accumulation.

why not accumulate errors?

error accumulation is valuable when:
- feedback loop is slow (compile, fix 10 errors, recompile)
- errors are independent (fixing one does not cascade)
- recovery is possible (skip to synchronisation point, continue)

protocol parsing lacks all 3:

**no feedback loop**
client sends request, server responds. no "fix and resend"
within the same parse. the response *is* the feedback.

**state corruption**
after a parse error, byte stream position is unknown.
if Content-Length is wrong, where does the body end?
if a header is malformed, where does the next header start?
pipelining makes this acute — the "next request" may already
be in the buffer, but we cannot find its boundary.

**recovery value: zero**
malformed request → 400 → done. client retries with a new request.
there is no benefit to finding "the next error" when no next
request can be safely located.

contrast with compilers:
- feedback loop: edit, compile, see errors, edit again
- independence: syntax error in function A does not affect function B
- recovery: skip to `}` or `;`, resynchronise, continue

fundamental difference: files are edited, protocols are streamed.


---


## system failures — runtime responsibility

the frontend never sees system failures.

Connection calls read(). if read() fails, Connection:
- detects the failure (return value, errno)
- logs the event
- closes the connection
- destroys the frontend instance

no bytes reach the frontend. no HTTP response is possible
if the socket is dead. the frontend is not involved.

the frontend's contract: given bytes, parse them.
acquiring bytes is not its concern.


---


## programming errors — assertions

internal invariants are protected by assertions.
```cpp
ParseResult HttpRequestFrontend::advance(const char* buf, size_t len)
{
    assert(phase_ != ParsePhase::Complete);  // precondition
    // ...
}
```

if an assertion fires, the program is wrong. no recovery.
abort with diagnostic in debug builds.

assertions document invariants. they are not error handling —
they are defect detection. a firing assertion means: fix the code.


---


## summary

| category    | mechanism      | rationale                              |
|-------------|----------------|----------------------------------------|
| protocol    | error code     | expected failure, code is the response |
| protocol    | fail-fast      | no recovery possible after corruption  |
| system      | (runtime)      | frontend never touches I/O             |
| programming | assertion      | defects are not handled, they are fixed|
