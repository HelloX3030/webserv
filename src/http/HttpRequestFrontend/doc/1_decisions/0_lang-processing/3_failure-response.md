## error strategy: fail-fast

### the question

should the parser accumulate multiple errors or stop at the first?

### the analysis

error accumulation is valuable when:
- feedback loop is slow (compile, fix 10 errors, recompile)
- errors are independent (fixing one doesn't cascade)
- recovery is possible (skip to synchronisation point, continue)

protocol parsing characteristics:
- no feedback loop: client sends request, server responds. no "fix and resend"
  within the same connection.
- state corruption: after a parse error, byte stream position is unknown.
  if Content-Length is wrong or missing, where does the next request start?
  pipelining makes this acute — the "next request" is already in the buffer.
- recovery value: zero. malformed request → 400 → done. client retries
  with a new request on a new connection (or same connection if keep-alive,
  but starting fresh).

contrast with compilers:
- feedback loop: edit, compile, see errors, edit again
- independence: syntax errors in function A don't prevent checking function B
- recovery: skip to `}` or `;`, resynchronise, continue

the difference is fundamental: files are edited, protocols are streamed.

### the decision

fail-fast. first error terminates parsing, returns error code.
no attempt to skip to "next request" — boundaries are unknowable
after parse failure.

### the principle

recovery requires recoverability.
protocol streams lack synchronisation points visible after corruption.
therefore: no recovery, immediate failure.
