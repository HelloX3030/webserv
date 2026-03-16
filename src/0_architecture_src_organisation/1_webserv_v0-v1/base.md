"base"

dependency floor (Fd RAII, logging, signals, utils)

domain-agnostic primitives: error types, string utilities, logging


name kept
What the name encodes:
"base" implies position in the dependency graph — this is the floor, nothing here depends on domain code.
It's a structural constraint that communicates what can and cannot live here.

better than "utils"
"utils" implies convenience — "stuff that's useful in multiple places".
This is a weaker predicate. "Utils" directories become dumping grounds
because "useful in multiple places" is true of almost everything interesting. There's no constraint.


The test:
When someone asks "should X go in base/?", the answer requires checking: "does X depend on any domain concept (HTTP, config, networking)?" If yes, it doesn't belong. The name encodes discipline.
When someone asks "should X go in utils/?", the answer is: "is X useful?" — always yes. No discipline.
However:
The contents of your current base/ deserve scrutiny:

Fd — genuinely domain-agnostic RAII for POSIX fds. ✓
format — string formatting. ✓
log — diagnostics infrastructure. ✓
utils — path resolution. ✓ (the function is generic even if HTTP uses it)

from v0, no longer in v1:
signal.cpp
  see: core/
