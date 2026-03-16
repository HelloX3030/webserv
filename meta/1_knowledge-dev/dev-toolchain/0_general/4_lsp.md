# lsp — language server protocol


## scope

this document maps the Language Server Protocol as the
architectural solution to the M×N editor-language coupling problem:
without a shared protocol, every combination of N languages and
M editors requires a bespoke integration. LSP factorises this into
N server implementations and M client implementations communicating
over a shared protocol.

to be developed here: the JSON-RPC wire protocol and message
taxonomy (request, response, notification); the server lifecycle
(initialize, capabilities negotiation, shutdown); the capability
model; language server implementations relevant to the languages
in this system — clangd (C++), rust-analyzer (Rust),
haskell-language-server (Haskell), agda-language-server (Agda);
editor client implementations — eglot (Emacs, built-in since 29,
lightweight), lsp-mode (Emacs, heavier feature set), neovim native
LSP; per-project configuration in Emacs via .dir-locals.el and
project.el.




## ctx

LSP — how Microsoft did this
Pragmatic necessity, not vision. They were building VS Code (2016), needed language intelligence for many languages, and didn't want to implement a separate plugin per language per editor. So they formalised what their TypeScript language server already did internally — request/response over JSON-RPC — into a protocol. Then open-sourced it.
The insight isn't novel: it's just the Unix philosophy applied to editor tooling. Separate the server (knows the language) from the client (knows the UI). The protocol is the interface. Microsoft had the right problem and enough adoption gravity to make it stick.
What made it land: VS Code became dominant fast. OmniSharp, clangd, rust-analyzer, pylsp all followed because the client base was there.


What GNU had before
All lexical, no semantics:

etags / ctags — scan source for identifiers, emit a tag file. regex-based. knows nothing about types, scopes, overloads. go-to-definition by text search.
cscope — C-specific cross-reference database. call graphs, symbol search. still lexical.
GNU Global (gtags) — more sophisticated tag system, incremental updates, multiple language support.
CEDET (Emacs) — the serious attempt. a full semantic parser inside Emacs, written in Elisp. understood C/C++ well enough for completion and navigation. but: Emacs-only, slow, perpetually lagging behind language evolution, maintained by one person.

The fundamental gap: all of these either operate lexically (fast, shallow) or re-implement the language frontend (slow, incomplete, duplicated effort). Neither approach scales to modern C++.
clangd solved this by being the compiler. Not a re-implementation — the actual clang frontend exposed as a library. The language intelligence is identical to what the compiler uses. There is no gap to drift.
