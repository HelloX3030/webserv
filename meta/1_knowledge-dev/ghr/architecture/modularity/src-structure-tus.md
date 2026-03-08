# translation units and source structure


## the compiler's unit of work

a translation unit (TU) is the atomic unit of compilation: 1 source
file after the preprocessor has run. the compiler sees exactly 1 TU
at a time — no knowledge of other TUs during compilation; only the
linker joins them.

the preprocessor runs before any parsing or type checking. it is a
text substitution engine. `#include` pastes the contents of another
file in place, verbatim. the result is 1 flat text stream — the TU
the compiler sees.

what a human sees as multiple files, the compiler may see as 1, if
those files are textually included into a single root.


---


## the tension: human decomposition vs compiler boundaries

a large module benefits from being split across files: separate
concerns, separate literate documents, separate editorial units.
c++'s default model makes each file a TU, and TUs cannot share names
from an anonymous namespace — anonymous namespace scope is precisely
1 TU.

if a module's implementation state must remain invisible to all
external TUs, it belongs in an anonymous namespace. if the
implementation is split across files compiled as separate TUs, none
of them can see the anonymous-namespace definitions of the others.

the apparent solutions and their costs:

class in public header — exposes internal types (even if private) in
a header seen by all includers. internal changes propagate
recompilation. falsely implies the module is a type, not a process.

internal header — correct encapsulation from external consumers, but
a file whose sole purpose is enabling a split that should not require
it. accidental complexity.

fragment inclusion — the correct solution, described below.


---


## fragment inclusion

files that are not compiled independently, but textually included into
1 root file. from the compiler's perspective, 1 TU. from the reader's
perspective, a collection of coherent, self-contained documents.

the orchestrator:
- defines all shared internal types in an anonymous namespace
- declares the public interface in a named namespace
- includes the fragment files

the fragment files:
- contain method definitions only
- carry no includes, no guards, no namespace declarations
- inherit the full context of the TU they are included into

the build system lists only the orchestrator as a compilation target.
fragment files are editorial units, not compilation targets. if a
fragment is accidentally compiled independently, the compiler errors —
the internal types it references are undefined.

SQLite uses this as its primary distribution format: over 100 source
files concatenated into sqlite3.c, 1 TU, 1 compilation target. the
split amalgamation (sqlite3-all.c) uses `#include` directly, for the
sole reason that some debuggers cannot handle source files over 32,767
lines. same pattern, same logic.


---


## applied: ConfigFrontend

ConfigFrontend.cpp is the only compilation target. it defines
TokenType, Token, and struct Frontend in an anonymous namespace,
then defines ConfigFrontend::parse() in the named namespace,
then includes the fragment files.

fragment files contain method definitions for Frontend only.
they carry no includes, no guards, no namespace declarations.
they compile as part of ConfigFrontend.cpp's TU, not independently.
if compiled independently, the compiler errors on the first
reference to Frontend or TokenType — the correct failure mode.

build system consequence: list ConfigFrontend.cpp only.
the fragment files must not appear as build targets.


---


## language perspectives

Haskell — no TU concept at module level. within a module, all
definitions are mutually visible without any class mechanism. internal
state is threaded via the State monad or abstracted by parser
combinator libraries (Parsec, Megaparsec).

Rust — the module system (mod, pub) maps cleanly. absence of pub is
the anonymous namespace equivalent. a module can span multiple files
via mod declarations — no fragment-inclusion workaround needed.

Agda — a file is a module; imports are explicit; internal definitions
private by default. no preprocessor, no TU concept in this sense.
the structural problem does not arise.


---


## references

Stroustrup, B. The C++ Programming Language. 4th ed. ch. 15.
    separate compilation, linkage, the one-definition rule.

Lakos, J. Large-Scale C++ Software Design. ch. 1-2.
    physical design: the cost of header dependencies.

sqlite.org/amalgamation.html
    the split amalgamation: fragment inclusion at scale.