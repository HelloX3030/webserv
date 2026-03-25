# failure — ontology

what failure IS, prior to classification, detection, or response.


---


## failure as contract deviation

a system fails when it deviates from its specification.

the specification may be:
- implicit: programmer intent, undocumented assumptions
- explicit: documentation, API contracts, requirements
- formal: types, proofs, verified properties

without a contract, "failure" has no meaning. a program that produces
arbitrary output is not "failing" unless we assert it should produce
something specific. failure is relational — it exists only between
a system and a specification of what that system should do.

this has an immediate consequence: the quality of failure detection
depends on the quality of specification. implicit contracts produce
ambiguous failures. formal contracts produce precise ones.

a buffer overflow is a "failure" only because we have an implicit
contract that memory access should remain within bounds. make this
contract explicit (array bounds in the type system, as in Agda), and
the failure becomes a compile-time error — or disappears entirely,
prevented by construction.

adversarial note: the specification itself can be incomplete, ambiguous,
or wrong. an attacker may exploit gaps between what the spec says and
what the programmer assumed. "correct to spec" does not mean "secure."


---


## failure as entropy increase

in a deterministic system with perfect information, there is no
failure — only state transitions. failure enters when we introduce:

1. intent: a distinction between desired and undesired states
2. entropy: the tendency of systems toward disorder

failure is an increase in system entropy relative to the intended
trajectory. information is lost. predictability decreases.

consider a function that throws an exception. the local execution
context — stack frames, local variables, intermediate computations —
is destroyed. information that existed at the throw site does not
reach the catch site. this is entropic loss.

contrast with a total function returning `Either Error Result`. the
error case preserves structure. the caller receives a value it can
inspect, propagate, or transform. information is retained.

the upstream goal in system design: minimise entropic loss at failure
boundaries. preserve information. make failure states as inspectable
as success states.

adversarial note: entropy is a weapon. an attacker induces failure to
destroy information (crash before evidence is logged), to increase
unpredictability (race conditions, undefined behaviour), or to force
the system into a less-ordered state that is easier to exploit.


---


## failure requires an observer

failure is not intrinsic to the system. it is a judgement made by an
observer comparing system behaviour against a specification.

in a standalone program, the observer is the programmer or operator.
in a distributed system, the observer may be another component — which
itself may fail. in a formally verified system, the observer is the
proof checker, but only for properties that were specified and proved.

this creates the observer problem: how do we know a system has failed
if the observation mechanism itself is fallible? in Genode and similar
microkernel architectures, this is addressed through hierarchy — each
component is observed by its parent, up to a trusted root. in Byzantine
fault tolerance, we assume some fraction of observers are lying and
design consensus protocols accordingly.

the epistemology of failure: we cannot know failure directly. we
observe symptoms — timeouts, unexpected values, violated assertions —
and infer failure. the quality of inference depends on the quality
of instrumentation.

adversarial note: the observer is an attack surface. if an attacker
can compromise the monitoring system, failures become invisible.
if an attacker can inject false observations, the operator responds
to phantoms while real attacks proceed undetected.


---


## the contract hierarchy

specifications exist at multiple levels:

mathematical: the function computes what its type signature claims.
in dependently typed languages, the type IS the specification.

logical: preconditions, postconditions, invariants. if the caller
provides valid input, the function produces valid output.

protocol: the system follows the grammar and semantics of its
communication protocols. HTTP requests are well-formed. TLS
handshakes complete correctly.

resource: the system operates within its resource budget. memory
allocations succeed. file descriptors do not exhaust.

timing: the system responds within acceptable latency bounds.
real-time systems have hard deadlines; soft systems have SLOs.

security: the system maintains confidentiality, integrity, and
availability. secrets do not leak. data is not corrupted. service
remains available.

failure at any level is still failure, but the severity and response
differ. a timing violation may be acceptable under load. a security
violation is never acceptable.

adversarial note: attackers probe for the weakest contract. if the
mathematical level is verified, attack the protocol level. if the
protocol is correct, attack timing (side channels) or resources
(exhaustion). defence requires contracts at every level.


---


## failure and correctness

a system is correct if it satisfies its specification for all valid
inputs. a system fails if it deviates from its specification for
some input.

but "correct" is not binary. we distinguish:

partial correctness: if the system terminates, it produces the right
answer. (it may not terminate.)

total correctness: the system terminates AND produces the right answer.

semantic correctness: the specification captures what we actually want.
(the spec itself may be wrong.)

a formally verified system is correct relative to its formalised spec.
if the spec is incomplete, correctness guarantees are incomplete. if
the spec is wrong, the system is "correctly wrong."

the deepest failures are specification failures. the system does
exactly what it was designed to do, but the design was flawed. these
failures are not detectable by the system itself — only by an external
observer with access to the true intent.

adversarial note: specification errors are gold for attackers.
the system "works correctly" while violating security properties
that were never formalised. the most dangerous vulnerabilities are
those where the code does exactly what it says.


---


## summary

failure is:
- deviation from contract (relational, requires specification)
- entropy increase (information loss, disorder)
- observer-dependent (requires judgement against intent)
- hierarchical (exists at multiple levels of abstraction)
- relative to correctness (which itself is relative to specification)

the upstream principle: clarity about specification precedes clarity
about failure. if you cannot state what the system should do, you
cannot state what it means for the system to fail.
