## pipeline terminology

the pipeline splits at a conceptual boundary.

frontend: everything operating on the source language.
    lexing, parsing, semantic analysis, IR construction.
    output: structured, language-independent representation.

backend: everything operating on the target.
    optimisation, code generation, linking.
    output: executable artefact.

the split point is the intermediate representation (IR).
Clang's frontend produces LLVM IR; LLVM backend consumes it.

webserv usage: ConfigFrontend, HttpRequestFrontend are frontends.
they parse source formats into structured C++ representations.
there is no backend — the representation is used directly at runtime.

---

future: expand on compiler architecture (phases, passes, IRs, SSA form, etc.)
