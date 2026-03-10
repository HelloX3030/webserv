# dev-toolchain — essence


## the problem

a compiler transforms source into machine artifacts.
it has everything it needs to do so: source files, flags,
include paths. it does its job and exits.

but a developer working on source needs more than compilation:
- which symbol is this identifier bound to?
- where is this function defined?
- what type does this expression have?
- which call sites will break if I change this signature?
- does this code contain a class of known bug?
- is this code formatted consistently?

none of these are compilation. they are *analysis* — inspection
of source code's semantic structure without producing artifacts.
the compiler has the information to answer them, but exposes none
of it by default.

this is the problem dev-toolchain solves.


---


## telos

provide source intelligence to the developer
by reusing the compiler's semantic machinery,
without coupling that machinery to any particular editor or IDE.


---


## the architectural gap

the compiler knows, for every translation unit (TU):
- which flags, include paths, and standard version govern it
- the full AST of the code
- the type of every expression
- every symbol, its definition site, its references

classical IDEs (Eclipse, Visual Studio) solved this by rebuilding
the compilation model themselves — a second, parallel implementation
of what the compiler does. this is expensive, incomplete, and
perpetually out of date.

the correct solution: give analysis tools access to the compiler's
own model, rather than re-implementing it.

clang's LibTooling and LibASTMatchers do exactly this. they expose
the clang compiler's internals — the AST, the semantic model,
the diagnostic engine — as a library. analysis tools built on
this library operate on the same representation the compiler uses.
no duplication. no drift.


---


## the interface: compile_commands.json

for a multi-file project, analysis tools need to know how each TU
is compiled. there is no single universal answer: flags differ
by file, by build variant, by platform.

`compile_commands.json` (the JSON Compilation Database) is the
standard interface. it records, for each TU:
- the source file
- the working directory
- the exact compiler invocation (all flags, include paths)

the build system — the authoritative source of this information —
generates it. analysis tools consume it. no other path is correct:
duplicating compilation flags in analysis tool config creates 2
sources of truth that diverge.

```
Makefile  ──────────────────────────────────────────┐
  (single source of truth for compilation flags)     │
                                                     ▼
                                        compile_commands.json
                                             │
                          ┌──────────────────┼──────────────────┐
                          ▼                  ▼                  ▼
                       clangd           clang-tidy        clang-check
                   (navigation)    (static analysis)    (diagnostics)
```


---


## the tools

### clangd
a language server implementing the Language Server Protocol (LSP).
telos: editor-agnostic source intelligence (completions,
go-to-definition, hover types, diagnostics, refactors).
config: `.clangd`

### clang-tidy
a linter and static analyser built on LibASTMatchers.
telos: detect semantic bugs and style violations the compiler
does not enforce — use-after-move, unchecked casts,
modernisation opportunities, security patterns.
config: `.clang-tidy`

### clang-format
a source formatter built on the clang lexer.
telos: normalise syntactic presentation — indentation, spacing,
brace placement — mechanically and without argument.
config: `.clang-format`

the 3 tools are independent. each addresses a distinct concern:
navigation, correctness, presentation.


---


## relation to build-systems

build-systems/ addresses the artifact pipeline:
source → (compilation) → binary.
telos: correct, minimal, reproducible artifact production.

dev-toolchain/ addresses the analysis pipeline:
source → (semantic analysis) → developer intelligence.
telos: source comprehension without artifact production.

they are related by `compile_commands.json`, which carries
compilation semantics from the build system into the analysis layer.
they are otherwise orthogonal.
