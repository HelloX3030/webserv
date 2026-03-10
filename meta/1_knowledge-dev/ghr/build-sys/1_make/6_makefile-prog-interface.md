# makefile — interface and scope of authority


## scope of authority

the Makefile is source of truth for the compilation graph:
every fact the compiler needs to process a translation unit.

what it owns:

- compiler and language standard (`CXX`, `-std=c++17`)
- diagnostic and correctness flags (`-Wall`, `-Wextra`, `-Werror`)
- include paths (`-I`)
- build variant flags (`-g`, `-O0`, `-DDEBUG=1`,
  `-fno-omit-frame-pointer`, sanitiser flags)
- source file discovery (`$(wildcard ...)`)
- dependency graph structure (`-MMD -MP`, pattern rules,
  `-include $(DEP_FILES)`)
- link flags (`LDFLAGS`)

what it does not own:

- analysis configuration: which checks clang-tidy runs,
  which diagnostics clangd surfaces
- formatting rules: indentation, brace style, line limits —
  clang-format operates on the token stream without reference
  to how any TU is compiled
- editor behaviour: LSP server startup, inlay hints, completion

the Makefile encodes what the compiler needs; analysis tools
and formatters are consumers of the compiler's model or output,
not inputs to it.


---


## the export interface

the Makefile's compilation semantics are not directly readable
by analysis tools. the export mechanism is `compile_commands.json`:
a per-TU record of the full compiler invocation — file, working
directory, flags, include paths — produced by intercepting
the build via bear.

`compile_commands.json` is the sole interface between the build
system and the analysis layer. all tools requiring compilation
semantics (clangd, clang-tidy) consume it exclusively.
clang-format does not consume it, as formatting is sub-semantic.

the corollary: analysis tool config (`.clangd`, `.clang-tidy`)
must never declare compilation flags. doing so creates a second
source of truth that diverges silently as the project evolves —
clangd then analyses code under assumptions the compiler does
not share.


---


## propagation: what a build change affects

a Makefile change propagates to analysis tools only after
`bear -- make` is re-run and `compile_commands.json` is refreshed.

changes requiring a refresh: source file added or removed,
include path changed, compiler flag changed, build variant selection changed.

changes that do not require a refresh: source file content edited
(the compilation graph is unchanged; the database records invocations,
not content), clang-tidy or clang-format config edited.

the Makefile's authority ends at the database. clangd staleness
beyond that boundary is resolved by clangd's own file-watching.


---


## see also

`dev-toolchain/` — the full pipeline from Makefile through
`compile_commands.json` to each consumer.
