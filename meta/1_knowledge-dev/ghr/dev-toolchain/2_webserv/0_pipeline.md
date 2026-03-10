# dev-toolchain — webserv


## instantiation of the general pipeline

see:
ghr/build-sys/1_webserv/
ghr/dev-toolchain/0_general/1_pipeline.md


---


## makefile: compilation flags (source of truth)

```makefile
CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17 -MMD -MP
INCLUDES := -I include
```

`-MMD -MP`: instructs the preprocessor to emit `.d` dependency
files alongside object files. these declare the actual header
dependencies of each TU, so Make tracks them correctly.
this is a build system concern, not an analysis concern —
the flags appear in `compile_commands.json` and are visible
to clangd, but clangd ignores them (it performs its own
include resolution).

build variants (`debug`, `leaks`) add:
```makefile
-DDEBUG=1 -g -O0 -fno-omit-frame-pointer
```

bear captures all variants if invoked against the corresponding
target. for analysis purposes, the release invocation is
sufficient — clangd needs the flags, not the debug symbols.


---


## generating the compilation database

```bash
bear -- make
```

run from the project root.
produces `compile_commands.json` in the project root.

refresh when: `.cpp` files added/removed, `CXXFLAGS` or
`INCLUDES` changed in the Makefile.


---


## .clangd

```yaml
CompileFlags:
  CompilationDatabase: .
```

no flags declared here. the Makefile is the source of truth.


---


## .clang-tidy

checks selected for a systems C++ project
(non-blocking I/O server, C++17, correctness-critical):

```yaml
---
Checks: >
  clang-analyzer-*,
  bugprone-*,
  modernize-use-nullptr,
  modernize-use-override,
  modernize-use-default-member-init,
  modernize-loop-convert,
  modernize-use-emplace,
  performance-unnecessary-copy-initialization,
  performance-move-const-arg,
  readability-const-return-type,
  readability-container-size-empty,
  readability-string-compare,
  -bugprone-easily-swappable-parameters,
  -clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling

WarningsAsErrors: >
  clang-analyzer-*,
  bugprone-use-after-move,
  bugprone-infinite-loop

HeaderFilterRegex: '(src|include)/.*'

CheckOptions:
  - key:   modernize-loop-convert.MinConfidence
    value: reasonable
```

suppressions rationale:
- `easily-swappable-parameters`: fires on legitimate POSIX socket
  API wrappers where argument order is fixed by the API.
- `DeprecatedOrUnsafeBufferHandling`: fires on `read()`/`write()`,
  which are required by the project spec (permitted syscall list).


---


## .clang-format

```yaml
BasedOnStyle: LLVM

IndentWidth:                       4
TabWidth:                          4
UseTab:                            Never

BreakBeforeBraces:                 Allman
ColumnLimit:                       100
AllowShortFunctionsOnASingleLine:  None

SortIncludes:                      false

SpaceBeforeParens:                 ControlStatements
MaxEmptyLinesToKeep:               1
NamespaceIndentation:              None
IndentCaseLabels:                  true
```

`SortIncludes: false` — held during active development
while include structure is in flux.
when the project stabilises, migrate to:
```yaml
SortIncludes:    true
IncludeBlocks:   Regroup
IncludeCategories:
  - Regex:    '^"'
    Priority: 1
  - Regex:    '^<'
    Priority: 2
```


---


## what is not version-controlled

`.vscode/settings.json` — editor preferences, not project contracts.
each developer maintains their own locally.
add to `.gitignore`.

the clang files are version-controlled:
they are project-level contracts, not editor preferences.
`compile_commands.json` is typically gitignored
(generated artifact, not source).
