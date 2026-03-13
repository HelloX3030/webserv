# dev-toolchain pipeline


## the dependency, stated precisely

analysis tools need compilation semantics.
the build system holds compilation semantics.
the pipeline is the chain of transforms
that moves that information from source to consumer.

```
Makefile
  │  sgl src of truth: flags, include paths,
  │  language standard, build variants
  │
  │  bear -- make
  │  (intercepts compiler invocations at LD_PRELOAD layer)
  v
compile_commands.json
  │  1 entry per TU.
  │  each entry: file, working directory, full compiler invocation.
  │  the compilation database — stable across editor sessions.
  │
  ├──── clangd          reads on startup, re-reads on change
  │     telos: LSP intelligence (navigation, types, diagnostics)
  │
  ├──── clang-tidy      reads per invocation
  │     telos: semantic static analysis
  │
  └──── clang-format    does NOT read this file
        telos: syntactic normalisation (lexical, not semantic)
        config: .clang-format only
```

clang-format is the outlier: formatting is a purely syntactic
concern. it requires no knowledge of how the TU is compiled.


---


## the 3 phases of the workflow


### phase 1: project initialisation

run once when the project is first set up.

```bash
bear -- make
```

produces `compile_commands.json` in the project root.
clangd discovers it automatically by walking upward from
each source file until it finds the database.

no further configuration is required for clangd to become
fully functional after this step.


### phase 2: development

normal build cycle: `make`.
bear is not involved. `compile_commands.json` is stable.

clangd runs as a background server, invoked by the editor
via LSP. it reads `compile_commands.json` once at startup
and indexes the project. subsequent queries (completions,
hover, go-to-definition) are served from that index.


### phase 3: database refresh

`compile_commands.json` goes stale only when the compilation
graph changes — not when source content changes.

refresh conditions:
- source file added or removed
- include path changed (-I flag)
- compiler flag changed (standard, warnings, defines)
- build variant flags changed

refresh: `bear -- make` again.
clangd detects the file change and re-indexes automatically.


---


## configuration files and their roles

`.clangd`
  configures the language server process itself.
  correct content when compile_commands.json is present:

  ```yaml
  CompileFlags:
    CompilationDatabase: .
  ```

  that is all. no flags duplicated here.
  the database supplies them.

`.clang-tidy`
  selects which checks to run, configures their parameters,
  declares which should be promoted to errors.
  operates on TUs resolved via compile_commands.json.

`.clang-format`
  purely syntactic. indentation, brace placement, line length.
  independent of compilation. no database involvement.


---


## the invariant

1 source of truth for compilation semantics: the Makefile.
everything downstream is derived.

violating this — duplicating flags in `.clangd`, in IDE config,
in shell scripts — creates multiple sources that diverge silently.
a flag added to the Makefile that is not propagated to `.clangd`
causes clangd to analyse code under different assumptions than
the compiler uses. the analysis is then unreliable.

the pipeline enforces the invariant structurally:
the Makefile generates the database, the database is consumed.
there is no other path for compilation semantics to enter the
analysis layer.
