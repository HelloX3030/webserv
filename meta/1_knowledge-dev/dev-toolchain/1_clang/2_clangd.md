server startup, indexing, background index. configuration via .clangd YAML (CompileFlags, Diagnostics, InlayHints, Completion keys). the fallback flags mechanism (when no compilation database exists). cross-references to LSP doc.




webserv:
```
CompileFlags:
  CompilationDatabase: .
```

No flag duplication. The Makefile is the single source of truth.
