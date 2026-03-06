# 1. abstract model

## the dependency graph, stated formally

let S be the set of all artifacts in a build:
sources (leaf nodes) and derived targets (internal nodes).

define the dependency relation R ⊆ S × S where
(A, B) ∈ R means "A directly depends on B":
A cannot be brought up-to-date without first
considering B.

the induced graph G = (S, R) must be a DAG.
if G contains a cycle, the build is undefined:
there is no valid topological ordering of rules,
and no consistent notion of "up-to-date" exists.

the build process is a restricted topological traversal:
for a requested target T, compute the subgraph
reachable from T under R, topologically sort it,
then for each node in that order: execute its rule
if and only if it is out-of-date with respect to
its immediate predecessors.

note the restriction: only the subgraph reachable from T.
this is the minimality invariant from 0_telos-and-ontology.md
made operational. we do not touch what T does not need.


## the 4 phases of a C++ build

a C++ build is not 1 transformation but a pipeline
of 4 logically distinct phases. each phase is a
different kind of transformation with a different
dependency structure. conflating them is an
ontological error with practical consequences.

### phase 1: preprocessing

    input:  one .cpp file + all transitively #included headers
    output: a single translation unit (TU) — a stream of tokens,
            macros expanded, includes substituted
    tool:   the preprocessor (cpp, invoked as part of the compiler)

this phase *resolves* the dependency graph for that TU.
the headers included determine which symbols are declared,
which macros are available. the output is a complete,
self-contained specification of what the compiler will see.

criticality: the dependency edges from a .o file to
header files are determined here and only here.
no other phase has this information. this is why
delegating dependency generation to the preprocessor
(via -MMD) is the only correct approach — examined in
3_make-semantics.md and 4_make-idioms-and-folklore.md.

security note: the #include mechanism is a textual
substitution with no integrity guarantees. a compromised
header (system or third-party) silently contaminates
all TUs that include it. hermetic builds (see §design space
below) mitigate this by pinning the exact version and
content hash of every included artifact. this is the
technical foundation of supply-chain security in builds.
reference: SLSA framework (slsa.dev); Nix/Guix as
existence proofs of hermetic, reproducible builds.

### phase 2: compilation

    input:  one TU (as produced by phase 1)
    output: one object file (.o) — machine code + symbol table
             + relocation entries
    tool:   the compiler backend (cc1plus in GCC, cc1 in clang)

each TU is compiled independently. this is the
fundamental unit of incrementality: if a .cpp file
and its headers are unchanged, its .o file need not
be recompiled.

the object file is not yet executable. it contains:
- machine code with unresolved external symbol references
  (calls to functions defined in other TUs)
- a symbol table declaring what this TU defines and uses
- relocation entries: placeholders for addresses to be
  filled in at link time

flags meaningful here: -O{0,1,2,3,s,z}, -g, -std=c++17,
-Wall/-Wextra/-Werror, -fsanitize (must also appear at
link), -fPIC (for shared libraries), -I (header search paths).

flags NOT meaningful here: -l, -L (library search/linking).
passing these at compile time is a phase category error.

### phase 3: archiving (static library creation)

    input:  a set of .o files
    output: a static library (.a file) — an archive of .o files
    tool:   ar (archiver)

a static library is not a linked binary. it is a
structured collection of .o files, each retaining its
own symbol table and relocation entries. the linker
selects only the .o files it needs from the archive
(those that provide symbols required by the binary).

this phase is optional in a simple build.
it becomes necessary when:
- you modularise a large project into internal libraries
  (e.g. libconfig.a, libhttp.a within webserv)
- you want to distribute pre-compiled code without sources
- you want the linker to perform selective inclusion
  (only linking the object files that resolve actual references)

archiving is semantically different from compiling
and linking. it has its own dependency structure:
a .a file depends on the .o files it archives,
not on their source .cpp files directly.

make syntax for archiving:
    $(AR) rcs libfoo.a $(OBJ_FILES)
    # r: insert/replace members
    # c: create archive if it doesn't exist
    # s: write an object-file index (equivalent to ranlib)

the .a file is a *derived* artifact with its own
staleness: it must be rebuilt if any of its member
.o files change. Make handles this correctly if the
dependency is declared.

for webserv: if config parsing, http parsing, and
the runtime layer are cleanly separated, archiving
each into an internal library (libconfig.a, libhttp.a,
libruntime.a) produces a cleaner link step and enables
independent rebuilds of each subsystem.

### phase 4: linking

    input:  a set of .o files and/or .a/.so files
    output: an executable or shared library (.so/.dylib)
    tool:   the linker (ld, invoked via the compiler driver)

the linker resolves all unresolved symbol references
from phase 2. it walks the input .o files and libraries,
matches defined symbols to their uses, assigns final
memory addresses, and emits a complete binary.

two linking modes:

**static linking:**
the linker copies the relevant .o files from .a archives
directly into the output binary. the resulting binary
is self-contained: it has no runtime dependency on
external library files.
consequence: larger binary; immune to library version
changes at runtime; no dynamic linker overhead.

**dynamic linking:**
the linker records a reference to a shared library
(.so on Linux, .dylib on macOS) but does not copy
its code. at runtime, the dynamic linker (ld.so)
resolves these references by locating the .so file.
consequence: smaller binary; shares library code
between processes; vulnerable to library substitution
at runtime.

security note: dynamic linking is a classic attack
surface. LD_PRELOAD, LD_LIBRARY_PATH, and RPATH
manipulation allow adversarial library substitution
— an attacker replaces a shared library with a
malicious version. this is the mechanism behind
several privilege escalation attacks.
references:
- "The Linux Programming Interface", Kerrisk, ch. 41-42
  (definitive treatment of shared libraries on Linux)
- CVE database entries for LD_PRELOAD hijacking
  (search: "LD_PRELOAD privilege escalation")
- "Hacking: The Art of Exploitation", Erickson,
  for the attacker's perspective on dynamic linker abuse

flags meaningful only at link time: -l (link library),
-L (library search path), -Wl,... (pass options to ld),
-rpath (embed runtime library search path in binary).

flags meaningful at both compile AND link time:
-fsanitize=* — the sanitiser needs to instrument code
at compile time AND link in its runtime at link time.
omitting it from the link step silently produces a
binary that crashes or behaves incorrectly.
this is a common Makefile error: sanitiser flags in
CXXFLAGS but not propagated to the link rule.


## the complete dependency graph for a C++ project

for a project with n source files and m headers:

    [.cpp_1, .hpp_1, ..., .hpp_k]  →  .o_1  ─┐
    [.cpp_2, .hpp_1, .hpp_j]        →  .o_2  ─┤
    ...                                         ├→  [.a_1, .o_n, ...]  →  binary
    [.cpp_n, .hpp_m]                →  .o_n  ─┘

each .o depends on exactly the headers transitively
included by its corresponding .cpp. this dependency
set is unique per TU and can only be determined
by the preprocessor. the job of the build system
is to maintain this graph accurately as headers
are added, removed, or modified.


## the design space of build systems

build systems can be characterised along several axes.
this clarifies what Make is and what it is not.

### axis 1: dependency declaration (explicit vs inferred)

**explicit:** the programmer declares all dependencies.
Make (without -MMD), plain shell scripts.
risk: human error. a missing dependency means
a stale build is not detected.

**compiler-assisted:** the compiler emits dependency
information as a side effect of compilation (-MMD).
this is Make with automatic dependency generation.
the dependency graph approaches correctness.

**fully automatic:** the build system tracks all
file accesses by all tools (via filesystem hooks,
strace, or similar). any file read by any tool
is automatically a dependency.
examples: Tup, Redo (djb), certain Bazel features.
this is the logically complete solution.

### axis 2: staleness detection (mtime vs content hash)

**mtime-based:** a target is stale if its mtime
is older than any prerequisite's mtime.
Make uses this model. it is a proxy for content change.
a file can be touched without changing (false stale)
or rebuilt to identical content (missed reuse).
examined in depth in 7_staleness-and-correctness-models.md.

**content-hash-based:** a target is stale if the
content hash of any input has changed since the
target was produced.
examples: Bazel, Shake, Nix.
this is the logically correct model.
consequence: identical inputs → cached output, always.
this enables distributed caching and true reproducibility.

### axis 3: hermeticity (open vs hermetic)

**open (Make):** the build can access anything on
the filesystem: system headers, environment variables,
external tools, the current working directory.
fast to set up. non-reproducible in general.
a build that "works on my machine" may fail elsewhere
because it depends on undeclared system state.

**hermetic (Bazel, Nix):** every input to every rule
must be declared. undeclared inputs are inaccessible
(sandbox enforcement). consequence: the build is
reproducible bit-for-bit across machines and time.
this is the gold standard for security and CI/CD.

security note: non-hermetic builds are a supply-chain
vulnerability. an undeclared dependency on a system
header, a tool in PATH, or an environment variable
creates an invisible attack surface. replacing any
of these with a malicious version contaminates the
build silently. hermetic builds make this attack
impossible by construction.
reference: SLSA (Supply-chain Levels for Software
Artifacts) framework, Google, 2021. slsa.dev.
also: "Reflections on Trusting Trust", Ken Thompson,
Communications of the ACM, 1984 — the foundational
paper on build-time trust and the limits of source
code inspection.

### axis 4: execution model (sequential vs parallel)

**sequential:** rules execute one at a time.
correct by construction (no race conditions).
slow for large projects.

**parallel (make -j N):** independent rules
(no dependency path between them) execute concurrently.
dramatically faster. introduces correctness risk:
if dependencies are underdeclared, two rules
may race on the same output file.
Make's parallel execution is sound only if the
dependency graph is complete and correct.
this is another argument for -MMD: an incomplete
graph that appears correct in sequential execution
may produce races under -j.

### axis 5: incrementality granularity

**file-level (Make, Ninja):** the unit of incrementality
is a file. if any part of a file changes, the file
is considered changed.

**semantic (some JVM build tools, Bazel for some languages):**
the unit is a semantic declaration. a change to a
function body that doesn't alter the function's
type signature does not trigger recompilation of
callers.
not achievable for C++ without heroic effort.
for C++, file-level is the practical limit.


## where Make sits in this space

Make (with proper -MMD usage) occupies:
- dependency declaration: compiler-assisted (incomplete
  for system headers without additional care)
- staleness: mtime-based (proxy, not exact)
- hermeticity: open (non-hermetic by design)
- execution: parallel-capable (-j), sound only with
  correct dependency graph
- incrementality: file-level

this is not a criticism — it is a map. Make's position
in the design space makes it: fast to set up, highly
portable, adequate for any project where reproducibility
is not a hard requirement, and correct when the
dependency graph is maintained accurately.

for projects requiring hermetic, reproducible, distributed
builds (production software at scale, security-critical
systems), the correct tool is Bazel, Nix, or similar.
understanding Make deeply is not a prerequisite to
using Bazel — it is a prerequisite to understanding
*why* Bazel was built and what problem it actually solves.


## key sources

### foundational papers

- peter miller, "recursive make considered harmful" (1997).
  the paper that systematically diagnosed the pathologies
  of recursive Make and proposed the non-recursive alternative.
  available: https://aegis.sourceforge.net/auug97.pdf
  essential reading before 5_build-variant-architectures.md.

- andrey mokhov, neil mitchell, simon peyton jones.
  "build systems à la carte" (2018). icfp 2018.
  the most rigorous theoretical treatment of build
  systems: a unified framework classifying Make, Excel,
  Shake, Bazel, and others. defines the key axes
  (static vs dynamic dependencies, self-tracking, etc.)
  formally. available: https://dl.acm.org/doi/10.1145/3236774
  read this after mastering Make; it will restructure
  your mental model of the entire space.

- eelco dolstra. "the purely functional software
  deployment model" (2006). phd thesis, utrecht university.
  the theoretical and practical foundation of Nix.
  defines hermetic builds with mathematical rigour.
  available: https://edolstra.github.io/pubs/phd-thesis.pdf

- ken thompson. "reflections on trusting trust" (1984).
  turing award lecture. on the impossibility of trusting
  a binary you did not compile yourself, and the limits
  of source-code inspection. directly relevant to
  supply-chain security.
  available: https://dl.acm.org/doi/10.1145/358198.358210

### reference documentation

- gnu make manual. https://www.gnu.org/software/make/manual/
  the authoritative reference for GNU Make.
  chapters 2-6 (rules, variables, conditionals) and
  chapter 10 (implicit rules) are essential.

- michael kerrisk. "the linux programming interface" (2010).
  chapters 41-42 for shared libraries, dynamic linking,
  LD_PRELOAD, and the dynamic linker. the definitive
  linux systems reference.

### security

- slsa framework. https://slsa.dev
  supply-chain levels for software artifacts.
  the current industry standard for build provenance
  and supply-chain integrity. directly relevant to
  any security-oriented engineering work.

- "how to shoot yourself in the foot with makefiles",
  various authors. a useful catalogue of Make pathologies
  and their root causes. search as a general reference;
  no single canonical source.