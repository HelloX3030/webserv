# monotonicity

## essence

a process that moves in 1 direction only.
never reverses. may pause, but cannot regress.

mathematically: a function f is monotonic iff

for all x ≤ y: f(x) ≤ f(y)  (monotonically increasing)
or
for all x ≤ y: f(x) ≥ f(y)  (monotonically decreasing)

the sequence of values forms a chain with no backtracking.


---


## why it matters

monotonicity guarantees progress.

a monotonic process cannot cycle infinitely — it either terminates or diverges.
if the codomain is bounded, termination is assured.

this transforms "will it finish?"
from empirical observation to mathematical certainty.


---


## manifestations

**computation — state machines**:

WebServ
parser phases advance monotonically.
REQUEST_LINE → HEADERS → BODY → COMPLETE.
no phase returns to a previous phase.
guarantees: parsing terminates; no infinite phase loops.

**computation — termination proofs**:
a function terminates if some measure decreases monotonically
toward a lower bound. structural recursion on natural numbers:
each recursive call has a smaller argument. eventually reaches 0.

**thermodynamics — entropy**:
in isolated systems, entropy increases monotonically.
the second law. time's arrow.

**economics — utility maximisation**:
rational agents choose actions that monotonically increase
expected utility (under classical assumptions).

**music — melodic contour**:
a passage with monotonically rising pitch creates tension,
expectation. the reversal releases it.

**type theory — subtyping**:
covariant type constructors preserve monotonicity of subtyping.
if A ≤ B, then F(A) ≤ F(B). contravariance reverses it.


---


## the exception pattern

monotonicity often admits explicit exceptions — deliberate
resets from outside the system.

WebServ: HttpRequestFrontend ("parser"):
`reset()` returns to REQUEST_LINE for connection reuse.
the parsing logic itself cannot cause this; external intervention can.

thermodynamics: entropy decreases locally when energy is added
from outside the system.

the pattern: monotonicity within the system's own operation;
external forces may reset.


---


## connection to order

monotonicity presupposes order on the domain and codomain.
without "less than", "increases" is meaningless.

order theory is the foundation. see `2_relation/order.md` [UPCOMING].


---


## strict vs non-strict

**strict**: always advances. f(x) < f(y) when x < y.
**non-strict**: may pause. f(x) ≤ f(y) when x ≤ y.


WebServ: HttpRequestFrontend ("parser"):
non-strict: phase may stay constant across
multiple `advance()` calls (waiting for bytes).


strict monotonicity is stronger — guarantees movement, not just
non-regression.


---


## references

order theory: Davey & Priestley, "Introduction to Lattices and Order"

termination: any text on program verification, structural induction

thermodynamics: the second law, Boltzmann
