# local procedures

a local procedure is a named computation defined inside another
computation, with access to the enclosing scope's bindings.

telos: name a repeated operation without promoting it to the global
namespace. the name gives the operation identity at call sites; the
scope restriction signals it has no meaning outside the enclosing context.


## the 3 things it eliminates

repetition: identical logic appearing n times, each a silent
divergence risk under future edits.

anonymous inlining: the operation exists but has no name — the reader
must reconstruct its intent from its body at every site.

parameter noise: state shared between the local procedure and its
enclosing context is accessible directly, not threaded through
signatures. see computation/state-threading.md.


## relationship to closures

a closure is a local procedure that captures bindings from its
enclosing scope. the captured bindings become implicit parameters —
present in the procedure's environment but absent from its signature.

capture by reference: the procedure operates on the original binding.
mutations are visible in the enclosing scope. requires the enclosing
scope to outlive the procedure.

capture by value: the procedure operates on a copy taken at definition
time. mutations are local. safe across lifetime boundaries.

the choice is a statement about ownership and mutation intent.


## language realisations

Agda: `where` and `let` bindings. purely functional — no mutation,
no capture-by-reference. a local definition is a name for an
expression; it may reference bindings in scope by substitution.

Haskell: `where` (definition-scoped) and `let`/`in` (expression-
scoped). same functional character. for local procedures that carry
state across calls, the State monad makes the threading explicit in the type.

Rust: nested closures with explicit capture modes (`move` for by-value).
nested `fn` items are also possible but cannot capture — they are
local in lexical scope only, not in environment. the borrow checker
enforces that captured references do not outlive the closure.

C++: lambdas. see lang/cpp/lambdas.md for capture mechanics.


## when not to use

if the operation has meaning outside the enclosing context, it is a
free function or method, not a local procedure. premature localisation
hides reusable logic.

if the captured state is large or the procedure escapes the enclosing
scope (stored, returned, passed to another thread), the implicit
coupling becomes a liability.
