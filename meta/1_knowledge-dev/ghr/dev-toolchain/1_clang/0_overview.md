LLVM/clang architecture.

clang as a library, not just a compiler.
why?

LibTooling (AST access), LibASTMatchers (pattern-matching on ASTs), LibFormat (formatting).

the key claim: all 3 tools operate on the same semantic representation the compiler uses —
no drift, no duplication.

contrast with classical IDEs re-implementing their own semantic model.







The 3 (.clangd, .clang-tidy, .clang-format) cover the entire daily development workflow. The broader LLVM toolset has more tools — clang-include-fixer, clang-doc, clang-rename, clang-query — but none of them use persistent project-level config files in the same sense. They are invoked ad hoc or via scripts, not configured at project root.
The 3 config files are the complete set of project-level contracts you version-control. are version-controlled — they are project-level contracts, not editor preferences.





The Makefile produces artifacts (.o, binary). That's the build system's telos — already in your build-systems/ docs.
The tools you're asking about (clangd, clang-tidy, clang-format) produce no artifacts. Their telos is different: to provide analysis and transformation services over source code, feeding information back to the developer rather than forward to the machine.
The pivot between these two worlds is compile_commands.json — a standardised, machine-readable export of how each translation unit is compiled. It is the interface between build and analysis.
So the conceptual hierarchy is:
source code
    │
    ├── build system (Make)
    │       telos: produce correct artifacts minimally
    │       SOT: compilation flags, dependency graph
    │       output → binary
    │
    └── analysis infrastructure
            telos: provide source intelligence to the developer
            requires: compilation semantics (compile_commands.json)
            tools: clangd (navigation), clang-tidy (static analysis),
                   clang-format (normalisation)
            output → developer cognition
