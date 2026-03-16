## this file:

### location:
The flags are compiler semantics — what the compiler is instructed to enforce, detect, emit. The Makefile merely carries them; it does not define what they mean. build-sys/ is about artifact pipeline mechanics: dependency tracking, rule structure, variant architecture. Compiler flags are not Make knowledge — they are clang/gcc knowledge that happens to be specified in the Makefile.
The natural home is therefore dev-toolchain/1_clang/ — because the flags govern both compilation and, crucially, what flows into compile_commands.json and therefore what clangd and clang-tidy see. They are the ceiling on analysis intelligence. That is a dev-toolchain concern, not a build-system concern.

### name:
As for the name — the flags span two distinct categories: diagnostic/warning flags (what the compiler detects and reports) and code-generation flags (what the compiler emits: debug symbols, optimisation level, sanitiser instrumentation). A document called compilation-flags.md covers both honestly. diagnostic-flags.md would be too narrow. cxx-flags.md is terse and accurate.



## notes to process:
-Wall and -Wextra together constitute a reasonable baseline. -Werror enforces zero-warning discipline — correct, demanding, and exactly right for a correctness-critical project. -std=c++17 establishes the language model. -MMD -MP are build-system instrumentation, not analysis-relevant.
This is a competent set. It is not what elite developers use. What's missing, and why each matters:
-Wshadow — warns when a local variable shadows a variable from an outer scope. in a system with callbacks, lambdas, and class hierarchies, shadowing is a live bug class. a member named fd shadowed by a local fd in a lambda compiles silently and produces wrong behaviour. this flag costs nothing on clean code.
-Wnon-virtual-dtor — warns when a class with virtual functions lacks a virtual destructor. for any hierarchy where you delete via base pointer (which you do in an event-driven server with polymorphic handlers), this is undefined behaviour. the flag is not optional on a project of this architecture.
-Wold-style-cast — warns on C-style casts (int)x. enforces static_cast, reinterpret_cast, const_cast — each of which makes the cast's semantics explicit and visible to both the developer and to analysis tools. C-style casts are opaque and can silently become reinterpret_cast in ways the author did not intend.
-Wconversion — warns on implicit numeric conversions that may lose data. POSIX network APIs return ssize_t, sizes come as size_t, file descriptors are int: the mixing is pervasive in systems code and a source of real bugs. this flag surfaces them. it is initially noisy on new codebases; the noise is the point.
-Wnull-dereference — warns when the compiler can statically prove a null dereference path. limited in scope (only what the compiler can prove at O0), zero false positives.
-Woverloaded-virtual — warns when a derived class function hides, rather than overrides, a base class virtual. catching the difference between overloading and overriding at compile time is critical when the interface evolves.
