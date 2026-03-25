# contract

what the frontend promises.
failure is deviation from this specification.

upstream: `meta/2_knowledge/failure/0_general/0_ontology.md`


---


## the function
```
advance : bytes → ParseResult
```

given a byte sequence, produce a parse result. the function is pure:
no I/O, no side effects beyond internal state mutation. Connection
calls `read()`, receives bytes, passes them here. the frontend never
touches the socket.
```cpp
ParseResult advance(const char* data, size_t len);
```

the input is raw bytes. the frontend makes no assumptions about their validity.
they may constitute valid HTTP, invalid HTTP, or arbitrary garbage.
the contract handles all cases.


---


## the result type
```
ParseResult = Complete HttpRequest
            | Incomplete
            | Failed ErrorCode
```

3 outcomes, exhaustive and mutually exclusive:

`Complete`: the byte stream contained a complete, valid HTTP request.
the `HttpRequest` struct is fully populated. Connection may proceed
to routing and execution.

`Incomplete`: the byte stream does not yet contain a complete request.
more bytes are needed. Connection continues reading and calls
`advance()` again with additional data.

`Failed`: the byte stream violates HTTP grammar or semantic constraints.
parsing cannot continue. the `ErrorCode` (400, 413, 501, 505) identifies
the violation class. Connection sends the corresponding error response
and closes or resets.

there is no fourth case. the function always returns one of these.


---


## what "failure" means here

from the ontology: failure is deviation from contract.

for this component, "failure" has two distinct meanings:

**protocol failure**: the input violates HTTP. the frontend returns
`Failed ErrorCode`. this is correct operation. the component
successfully detected invalid input and reported it. the frontend
did not fail; the input did.

**component failure**: the frontend itself malfunctions. it enters an
impossible state, violates its own invariants, produces incorrect
output for valid input. this is a bug. the code is wrong.

the distinction matters:
- protocol failure: expected, handled, returns error code
- component failure: unexpected, unhandled, aborts via assertion

the frontend is designed to handle protocol failures. it is not
designed to handle its own bugs — those must be fixed, not handled.


---


## boundaries

**upstream boundary**: Connection owns the socket. it calls `read()`,
detects I/O errors, manages timeouts. bytes reach the frontend only
after successful read. system failures (socket errors, resource
exhaustion) are invisible here.

**downstream boundary**: the frontend produces `HttpRequest` or
`ErrorCode`. it does not send responses, close connections, or log.
those are Connection's responsibilities.

the frontend is a pure transformation in the middle. its contract is
narrow: bytes in, parse result out. nothing else.


---


## invariants

properties that must hold if the code is correct:

`phase_` progresses monotonically:
REQUEST_LINE → HEADERS → BODY → COMPLETE (or ERROR).
it never moves backward except via `reset()`.

`body_remaining_` never exceeds `max_body_size_`. the check happens
at the HEADERS → BODY transition.

`buffer_` indices are always within bounds. `extract_line()` and
`consume_line()` operate on positions returned by `find_crlf()`.

`error_code_` is set if and only if `phase_ == ERROR`.

violation of any invariant is a bug. detection: assertions.
response: abort. see `2_invariants.md`.


---


## guarantees

if the code is correct:

1. valid HTTP input eventually produces `Complete HttpRequest`
2. invalid HTTP input produces `Failed ErrorCode` with appropriate code
3. incomplete input produces `Incomplete` until sufficient bytes arrive
4. no input causes undefined behaviour, crash, or hang
5. memory usage is bounded by `max_body_size_` plus header overhead

these are the testable properties. a test suite exercises inputs and
verifies outputs match expectations. formal verification would prove
these hold for all inputs.


---


## non-guarantees

the frontend does not guarantee:

- protection against resource exhaustion upstream (Connection's domain)
- security of downstream processing (executor's domain)
- correctness of the HTTP specification itself

the frontend implements HTTP/1.1 as specified in RFC 9110 and 9112.
if the specification contains ambiguities or security weaknesses,
the frontend inherits them. "correct to spec" is not "secure."
