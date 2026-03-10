# dev-toolchain — essence


## the problem

a compiler transforms source into machine artifacts.
it has everything it needs: source files, flags, include paths.
it does its job and exits.

a developer working on source needs more than compilation:
which symbol is this identifier bound to?
where is this function defined?
what type does this expression have?
which call sites break if this signature changes?
does this code contain a known class of bug?

none of these are compilation. they are analysis — inspection
of source structure without producing artifacts. the compiler
has the information to answer them; it exposes none of it
by default.

this is the problem dev-toolchain solves.


---


## telos

provide source intelligence to the developer by reusing the
compiler's semantic machinery, without coupling that machinery
to any particular editor or IDE.


---


## the architectural gap

classical IDEs solved source intelligence by rebuilding the
compilation model themselves — a second, parallel implementation
of what the compiler does. this is expensive, incomplete, and
perpetually out of date.

the correct solution: give analysis tools access to the compiler's
own model. this is the architectural decision each modern toolchain
has arrived at independently: clang via LibTooling, rustc via
rust-analyzer's use of compiler internals, GHC via the GHC API
consumed by haskell-language-server. the details differ; the
principle is the same.


---


## the interface: compilation database

for a multi-file project, analysis tools need to know how each
translation unit (TU) is compiled. flags differ by file, by
build variant, by platform. there is no single static answer.

a compilation database is a per-TU record of: source file,
working directory, full compiler invocation. the build system —
sole authoritative source of compilation semantics — generates it.
analysis tools consume it. no other path is correct.

the JSON Compilation Database (compile_commands.json), specified
by the LLVM/Clang project, is the de facto standard for C/C++
toolchains. other ecosystems have equivalent mechanisms:
rust-analyzer reads Cargo's build graph directly; HLS reads
cabal/stack project files.

the deeper principle: a single source of truth for compilation
semantics. everything downstream is derived. duplicating flags
in analysis tool config creates multiple sources that diverge
silently.


---


## the tools (clang instantiation, C/C++)

the 3 tools composing the analysis layer for C/C++ projects:

clangd — a language server (LSP): editor-agnostic source
intelligence (completions, navigation, types, diagnostics).

clang-tidy — a linter: semantic bug and style detection
beyond what the compiler enforces.

clang-format — a formatter: mechanical syntactic normalisation,
independent of compilation semantics.

each addresses a distinct concern. their shared architectural
basis — all built on the same compiler AST — is treated in
1_clang/.


---


## relation to build-systems

build-systems/ addresses the artifact pipeline:
source → (compilation) → binary.
telos: correct, minimal, reproducible artifact production.

dev-toolchain/ addresses the analysis pipeline:
source → (semantic analysis) → developer intelligence.
telos: source comprehension without artifact production.

they share 1 interface: the compilation database, which carries
compilation semantics from the build system into the analysis layer.
they are otherwise orthogonal.
