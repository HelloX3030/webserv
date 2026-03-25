# assertions

explicit checks that a condition holds, aborting if violated.
a detection mechanism for programming errors.


---


## the concept

an assertion states: "this condition must be true at this point in
execution. if it is not, the program is wrong."
```
assert(index < array_length);
assert(phase != COMPLETE);
assert(pointer != null);
```

if the condition holds, execution continues. if it fails, the program
aborts immediately with a diagnostic.

assertions detect errors (invalid states) before they become failures
(observable deviations). they are part of the fault → error → failure
chain: a fault (bug) causes an error (violated invariant), which an
assertion catches before it propagates to failure.


---


## what assertions are NOT

assertions are not error handling.

error handling addresses runtime conditions: file not found, network
timeout, invalid user input. these are expected. the program is
correct; the environment is uncooperative.

assertions address programming errors: violated invariants, impossible
states, logic bugs. these are unexpected. the program is incorrect.

the distinction:
- error handling: "this might happen, here is what we do"
- assertion: "this must never happen, if it does we are broken"

conflating them produces two pathologies:

1. using assertions for runtime conditions. the assertion is disabled
   in release builds; the check disappears; the program misbehaves.

2. using error handling for bugs. the program "recovers" from an
   impossible state, masking the defect, continuing in an undefined
   condition.


---


## the three assertion types

**preconditions**: what must be true when a function is entered.
the caller's obligation. if violated, the caller is buggy.
```
// caller must not advance a completed parser
assert(phase_ != ParsePhase::COMPLETE);
```

**postconditions**: what must be true when a function exits.
the callee's obligation. if violated, the function is buggy.
```
// after successful parse, request must have a method
assert(!result.request.method.empty());
```

**invariants**: what must be true at all times for a data structure
or system. if violated at any point, something is buggy.
```
// body_remaining_ must never exceed max_body_size_
assert(body_remaining_ <= max_body_size_);
```

in formal methods, these correspond to Hoare logic:
`{P} S {Q}` — if precondition P holds before statement S,
postcondition Q holds after.


---


## language mechanisms

**C/C++**: `assert()` macro from `<cassert>`. evaluates the condition;
if false, prints diagnostic and calls `abort()`. disabled when `NDEBUG`
is defined (typically in release builds).
```cpp
#include <cassert>
assert(ptr != nullptr);
```

**Rust**: `assert!()` macro, always active. `debug_assert!()` for
debug-only checks (analogous to C's NDEBUG behaviour).
```rust
assert!(index < len);
debug_assert!(invariant_holds());
```

**Haskell**: `assert` from `Control.Exception`. in GHC, controlled by
`-fignore-asserts`. less common; preconditions often encoded in types.
```haskell
assert (length xs > 0) (head xs)
```

**Agda/dependent types**: assertions dissolve into the type system.
a function that requires a non-empty list takes `Vec A (suc n)` — you
cannot call it with an empty list. the precondition is the type.
there is no runtime check because invalid calls do not compile.


---


## when assertions should fire

an assertion firing means: the program is wrong. not "the input is
bad" or "the network failed" — the code itself is defective.

correct response: abort. produce diagnostic. fix the code.

incorrect responses:
- catch the assertion and continue (masks the bug)
- log and proceed (undefined behaviour follows)
- disable assertions in production and hope (the bug remains)

the purpose of aborting is not to "handle" the error but to make the
defect visible, immediately, loudly, with maximal diagnostic context.

a program that continues past a violated invariant is in an undefined
state. any subsequent behaviour is meaningless. better to crash now
with a clear message than corrupt data silently.


---


## assertions in release builds

a contentious question: should assertions remain active in production?

**arguments for disabling** (the C/C++ default with NDEBUG):
- performance cost of checks
- assertion messages may leak internal details
- "if testing is thorough, bugs are caught before release"

**arguments for keeping**:
- bugs escape testing
- silent corruption is worse than crashing
- crash with diagnostic beats undefined behaviour
- performance cost is usually negligible

the security perspective complicates this:

a crash is a denial of service. an attacker who can trigger an
assertion can take down the service. this argues for disabling
assertions — or for designing systems where component crashes
are contained (supervision trees, error kernels).

an assertion message is information leakage. file paths, line numbers,
variable values — all useful for reconnaissance. this argues for
opaque crash handling in production.

but: silent continuation past a violated invariant may be worse.
buffer overflows, memory corruption, authentication bypasses — these
are the consequences of "handling" impossible states instead of aborting.

there is no universal answer. the decision depends on:
- the cost of crash vs the cost of corruption
- the isolation model (can crashes be contained?)
- the information sensitivity of diagnostics
- the maturity of the codebase


---


## what to assert

assert conditions that:
1. should be true if the code is correct
2. are not already enforced by the type system
3. are cheap to check relative to the operation
4. provide useful diagnostics when violated

**good assertions**:
```cpp
assert(phase_ != ParsePhase::COMPLETE);   // precondition
assert(googol_pos <= buffer_.size());      // bounds invariant
assert(googol_remaining_ <= max_body_size_); // value invariant
```

**poor assertions**:
```cpp
assert(googol != nullptr);  // if type is reference, cannot be null
assert(parse() == SUCCESS); // this is control flow, not invariant
assert(googol.validates());  // expensive check, do separately
```

the test: "if this assertion fails, is it definitely a bug in this
codebase?" if yes, assert. if it might be bad input or environmental
failure, use error handling instead.


---


## assertions and formal verification

assertions are informal specifications. they state properties but do
not prove them.

in Agda, Coq, or Lean, the goal is to prove that assertions can never
fail — that the preconditions are always satisfied by all callers,
that the invariants are preserved by all operations.

when you prove a property, the runtime check becomes redundant. the
type system guarantees it. assertions are the pragmatic approximation
for languages and contexts where proof is not (yet) available.

the trajectory:
1. implicit assumption (nothing checks)
2. assertion (runtime check, abort if violated)
3. test (check specific cases)
4. proof (check all cases, compile-time)

each step increases assurance. assertions are step 2: better than
nothing, less than proof.


---


## adversarial considerations

**DoS via assertion**: if an attacker can craft input that triggers
an assertion, they can crash the service. assertions should only
check invariants that valid operation cannot violate. if external
input can affect the condition, it is not an invariant — it is
input validation, and belongs in error handling.

**information leakage**: assertion messages reveal internal structure.
"assertion failed: phase_ == HEADERS, file parser.cpp, line 142"
tells an attacker about state machine design, file layout, line
numbers. in production, consider opaque crash reports that log
details internally but expose nothing to the client.

**assertions as specification disclosure**: the set of assertions
documents what the programmer believed to be invariants. an attacker
reading the source can identify what is NOT asserted — and target
those unprotected assumptions.

the security principle: assertions are internal. they should never
be reachable via external input under correct operation. if they
are, the boundary between "program correctness" and "input validity"
has been drawn in the wrong place.


---


## summary

assertions detect programming errors by checking conditions that
must hold if the code is correct. they are not error handling.
they abort on violation because continuation is undefined.

use assertions for:
- preconditions (caller obligations)
- postconditions (callee obligations)
- invariants (always-true properties)

do not use assertions for:
- input validation (use error handling)
- expected runtime failures (use return values, exceptions)
- expensive checks in hot paths (benchmark first)

the deeper goal: replace assertions with proofs. until then,
assertions are the pragmatic checkpoint between "hope it works"
and "proved it works."
