LLVM/clang architecture.

clang as a library, not just a compiler.
why?

LibTooling (AST access), LibASTMatchers (pattern-matching on ASTs), LibFormat (formatting).

the key claim: all 3 tools operate on the same semantic representation the compiler uses —
no drift, no duplication.

contrast with classical IDEs re-implementing their own semantic model.







The 3 (.clangd, .clang-tidy, .clang-format) cover the entire daily development workflow. The broader LLVM toolset has more tools — clang-include-fixer, clang-doc, clang-rename, clang-query — but none of them use persistent project-level config files in the same sense. They are invoked ad hoc or via scripts, not configured at project root.
The 3 config files are the complete set of project-level contracts you version-control. are version-controlled — they are project-level contracts, not editor preferences.
