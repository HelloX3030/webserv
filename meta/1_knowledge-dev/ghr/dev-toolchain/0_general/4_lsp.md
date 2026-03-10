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
