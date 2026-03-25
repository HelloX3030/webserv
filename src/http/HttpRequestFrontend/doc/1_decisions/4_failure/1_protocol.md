# protocol violations

invalid input is expected. handling it correctly is the frontend's
primary purpose.

upstream: `meta/2_knowledge/failure/0_general/2_taxonomy` [UPCOMING]


---


## the situation

clients are untrusted. the network is noisy. malformed requests arrive.
this is not exceptional — it is routine operation.

the frontend exists precisely to distinguish valid from invalid input.
returning `Failed ErrorCode` for invalid HTTP is correct behaviour,
not failure of the component.

the contract:
- valid input → `Complete HttpRequest`
- invalid input → `Failed ErrorCode`

both outcomes fulfil the specification.


---


## why return codes, not exceptions

3 reasons:

**protocol violations are expected.**
exceptions signal exceptional conditions. malformed HTTP is not
exceptional in a server receiving requests from the internet.
using exceptions for routine conditions conflates error handling
with defect detection.

**the error code IS the response.**
HTTP defines status codes for parse failures: 400 (bad request),
413 (payload too large), 501 (not implemented), 505 (version not
supported). the frontend produces exactly what the response needs.
no translation layer. no mapping from exception types to codes.

**control flow remains local.**
the caller inspects the result and decides: log, send response,
close connection. all visible at the call site. no non-local jumps.
no hidden control flow through exception unwinding.
```cpp
ParseResult result = frontend.advance(buf, len);
if (result.status == ParseStatus::Failed)
{
    send_error_response(result.error_code);
    close_connection();
}
```

the decision happens where the context exists.


---


## fail-fast

first violation terminates parsing. no recovery. no error accumulation.

**why not accumulate errors?**

error accumulation is valuable when:
- feedback loop is slow (compile, fix 10 errors, recompile)
- errors are independent (fixing 1 does not cascade)
- recovery is possible (skip to synchronisation point, continue)

protocol parsing lacks all 3.

**no feedback loop.**
client sends request, server responds. no "fix and resend" within
the same parse. the response IS the feedback. 1 error code suffices.

**state corruption.**
after a parse error, byte stream position is unknown. if
`Content-Length` is wrong, where does the body end? if a header is
malformed, where does the next header start? pipelining makes this
acute — the "next request" may already be in the buffer, but the
boundary cannot be found.

**recovery value: zero.**
malformed request → 400 → done. client retries with a new request.
there is no benefit to finding "the next error" when no next request
can be safely located.

contrast with compilers:
- feedback loop: edit, compile, see errors, edit again
- independence: syntax error in function A does not affect function B
- recovery: skip to `}` or `;`, resynchronise, continue

fundamental difference: files are edited, protocols are streamed.
a file can be corrected in place. a stream cannot be rewound.


---


## the error codes

each code maps to a specific violation class:

`400 Bad Request`: malformed syntax. request-line missing SP, header
missing colon, invalid characters, missing Host in HTTP/1.1,
Content-Length with differing duplicate values.

`413 Content Too Large`: body exceeds `max_body_size_`. detected at
HEADERS → BODY transition for Content-Length, or during chunk
accumulation for Transfer-Encoding: chunked.

`501 Not Implemented`: unsupported method. the server implements
GET, POST, DELETE. other methods receive 501.

`505 HTTP Version Not Supported`: version other than HTTP/1.0 or
HTTP/1.1.

see `3_error-codes.md` for full mapping.


---


## adversarial perspective

protocol handling is an attack surface.

**malformed input as probe.**
attackers send deliberately malformed requests to observe responses.
error messages, timing differences, and connection behaviour reveal
implementation details. the frontend produces minimal information:
a status code, nothing more. Connection sends a brief response body;
the frontend does not control its content.

**resource exhaustion via parsing.**
slowloris: send headers slowly, 1 byte at a time, hold connections
open indefinitely. defence is upstream (Connection timeouts), not
in the frontend. the frontend processes whatever bytes arrive.

**smuggling via ambiguity.**
conflicting Content-Length values, malformed chunk sizes, header
injection via CRLF in values. the frontend rejects ambiguous input
rather than guessing intent. fail-fast is a security property: do
not proceed with uncertain state.

**fuzzing resilience.**
arbitrary byte sequences must not crash the frontend, leak memory,
or cause undefined behaviour. every code path must terminate in
a defined result: Complete, Incomplete, or Failed. this is testable
and, in principle, provable.


---


## summary

protocol violations are expected input, not component failure.
the frontend handles them by returning error codes.

return codes instead of exceptions: the code IS the response,
control flow is local, protocol errors are not exceptional.

fail-fast instead of accumulation: no recovery is possible in a
corrupted stream, no benefit to finding additional errors.

adversarial stance: reject ambiguity, minimise information leakage,
ensure all byte sequences terminate in defined results.
