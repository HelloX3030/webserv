the style system (BasedOnStyle as a base, overrides). the key formatting decisions: brace style (Allman vs Attach), column limit semantics, include sorting. the // clang-format off/on escape. how it integrates into workflow (pre-commit, editor on-save)



what clang-format is operating on
clang-format works on the token stream — lexical, not semantic. it knows tokens, whitespace, and nesting depth. it does not know types, control flow, or meaning. this determines what it can and cannot do, and should constrain what we ask of it.







BasedOnStyle mechanics
clang-format has ~200 configurable options. specifying all of them in every project's .clang-format would be verbose and fragile (new clang-format versions add new options; yours would silently inherit defaults for them rather than your chosen base). BasedOnStyle sets all ~200 options to a named preset's values, then your file overrides only what you explicitly name. the named presets are: LLVM, Google, Chromium, Mozilla, WebKit, Microsoft, GNU. they correspond to the published style guides of those projects. LLVM is the correct neutral base: it is the clang project's own style, well-maintained, no corporate product idioms, and conservative defaults for anything you don't override.
practical consequence: any option you do not name in your file inherits from LLVM. if a future clang-format version adds a new option, it gets the LLVM value — a known, sane choice, not an arbitrary default.
