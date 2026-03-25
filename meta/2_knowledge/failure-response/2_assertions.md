Ontology: An assertion is a statement about what must be true at a given program point. If the statement is false, the program is wrong — not "encountered an error", but contains a defect.
The distinction:

Error handling — expected failures (bad input, network down, file missing). Recover or report gracefully.
Assertion — programming defects (violated invariant, impossible state reached). No recovery — the code itself is wrong.

In C/C++:

```cpp
#include <cassert>

void process(int* ptr)
{
    assert(ptr != nullptr);  // precondition: caller must provide valid pointer
    // if ptr IS nullptr, program aborts immediately
}
```

Behaviour:

debug build (NDEBUG not defined): assertion failure → abort() with diagnostic
release build (NDEBUG defined): assertion is removed entirely, no runtime cost


---

Why this matters for HttpRequestFrontend:
The failure strategy doc (1_decisions/4_failure/1_strategy.md:100-115) describes:
cppParseResult HttpRequestFrontend::advance(const char* buf, size_t len)
{
    assert(phase_ != ParsePhase::Complete);  // precondition
    // ...
}
This says: "if Connection calls advance() after parse is already complete, that's a bug in Connection, not a recoverable error."
Currently: no assertions in the implementation. The doc describes intent; code doesn't enforce it.
