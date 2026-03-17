# fragment extension: .inc


## the problem

wildcard-based source discovery treats every `.cpp` file as a
compilation target. fragment files are not compilation targets —
they are textually included into 1 orchestrator. the compiler
receives them as part of that TU.

if the build system compiles a fragment independently, it fails:
the fragment references types defined in the orchestrator's
anonymous namespace (Frontend, TokenType), which are undefined
outside that TU. this is the correct failure mode — but it is
better not to attempt the compilation at all.

a recursive wildcard such as

```makefile
rwildcard = $(foreach d,$(wildcard $1*),\
              $(call rwildcard,$d/,$2)\
              $(filter $(subst *,%,$2),$d))

SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)
```

descends unconditionally. depth limits do not exist; a `fragments/`
subdirectory provides no protection. the filter pattern `*.cpp`
is the only discriminant the build system has.


---


## the solution

the preprocessor is a text substitution engine. it has no concept
of file extension. `#include "f.inc"`, `#include "f.cpp"`,
`#include "f.fragment"` are identical in behaviour: the file's
contents are pasted verbatim at the inclusion site. extension is
a convention for tools, not a semantic distinction.

Webserv: fragments receive the extension `.inc` (included fragment).

the wildcard pattern `*.cpp` no longer matches them. they are
invisible to source discovery. the orchestrator includes them
explicitly by relative path. the build system is correct without
any filter logic, regardless of directory depth.

```
ConfigFrontend.cpp       ← compilation target: matched by *.cpp
ConfigFrontend_0_read.inc         ← fragment: not matched
ConfigFrontend_1_tokenise.inc     ← fragment: not matched
ConfigFrontend_2a_parse_navigate.inc
...
```

the fragment header comment updates accordingly:

```cpp
/* fragment: no includes, no guards. context: ConfigFrontend.cpp. */
```

this remains accurate and is now also structurally enforced:
attempting to compile the fragment independently gives a linker
error before the compiler is even invoked, because no build rule
exists for `.inc` files.


---


## precedent

`.inc` — SQLite split amalgamation, LLVM tablegen output, GCC
machine description includes. the convention predates C++11.

`.ipp` — used in some header-only and template libraries to
separate template definitions from declarations. implies
"implementation part of a header". less appropriate here: these
files are not template implementations.

`.inc` is the correct choice: it signals "this file is included,
not compiled" without implying a specific relationship to headers or templates.


---


## invariant

the build system's compilation model and the source's logical
structure must agree. the extension is the interface between them.
naming a fragment `.cpp` creates a false claim: that the file is
a compilation target. `.inc` states the truth.

this holds unconditionally for any depth of directory tree, any
number of fragments, any wildcard strategy that filters on `.cpp`.


---


## language perspectives

Agda — no preprocessor; no TU concept. a file is a module.
the structural problem does not arise; neither does the naming question.

Haskell — no preprocessor inclusion. Template Haskell splice files
use `.hs` uniformly; the module system, not the file extension,
determines compilation boundaries.

Rust — `mod` declarations control compilation. a file included via `mod foo;`
is a module, not a fragment; it is compiled as part of the crate
but has its own namespace. the `.rs` extension is uniform. no analogous naming problem.

the `.inc` convention is specific to the C preprocessor model.
in languages with principled module systems, it does not appear.
