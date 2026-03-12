# build phases


## the pipeline

a C++ build is not 1 transformation but 4 logically distinct phases.
each phase is a different kind of transformation with a different
dependency structure. conflating them is an ontological error with
practical consequences.

    preprocessing       .cpp + headers → translation unit (TU)
    compilation         TU → object file (.o)
    archiving           .o files → static library (.a)       [optional]
    linking             .o files + libraries → binary


---


## phase 1: preprocessing

    input       one .cpp file + all transitively #included headers
    output      translation unit — token stream, macros expanded,
                includes substituted
    tool        the preprocessor (cpp, invoked as part of the compiler)

this phase resolves the dependency graph for that TU. the headers
included determine which symbols are declared, which macros available.
the output is a complete, self-contained specification of what the
compiler will see.

criticality: the dependency edges from a .o to header files are
determined here and only here. no other phase has this information.
this is why delegating dependency generation to the preprocessor
(via -MMD) is the only correct approach.

security note: #include is textual substitution with no integrity
guarantees. a compromised header — system or third-party — silently
contaminates all TUs that include it. hermetic builds mitigate this
by pinning exact version and content hash of every included artifact.
this is the technical foundation of supply-chain security in builds.

references: SLSA framework (slsa.dev); Nix and Guix as existence
proofs of hermetic, reproducible builds.


---


## phase 2: compilation

    input       one TU (as produced by phase 1)
    output      one object file (.o) — machine code + symbol table
                + relocation entries
    tool        compiler backend (cc1plus in GCC, cc1 in clang)

each TU compiles independently. this is the fundamental unit of
incrementality: if a .cpp and its headers are unchanged, its .o
need not be recompiled.

the object file is not yet executable. it contains:

    machine code        with unresolved external symbol references
                        (calls to functions defined in other TUs)
    symbol table        what this TU defines and uses
    relocation entries  placeholders for addresses filled at link time

flags meaningful here: -O{0,1,2,3,s,z}, -g, -std=c++17,
-Wall/-Wextra/-Werror, -fsanitize, -fPIC, -I.

flags not meaningful here: -l, -L (library search/linking).
passing these at compile time is a phase category error.


---


## phase 3: archiving

    input       a set of .o files
    output      static library (.a) — an archive of .o files
    tool        ar (archiver)

a static library is not a linked binary. it is a structured collection
of .o files, each retaining its own symbol table and relocation entries.
the linker selects only the .o files it needs — those providing symbols
required by the binary.

this phase is optional in simple builds. necessary when:

- modularising into internal libraries (libconfig.a, libhttp.a)
- distributing pre-compiled code without sources
- enabling selective inclusion (linker pulls only what resolves
  actual references)

archiving has its own dependency structure: a .a depends on its member
.o files, not on their .cpp sources directly.

    $(AR) rcs libfoo.a $(OBJ_FILES)
    # r: insert/replace members
    # c: create archive if nonexistent
    # s: write object-file index (equivalent to ranlib)

the .a is a derived artifact with its own staleness: rebuild if any
member .o changes.


---


## phase 4: linking

    input       .o files, static libraries (.a), shared libraries (.so)
    output      executable or shared library
    tool        linker (ld, invoked via compiler driver)

the linker resolves all external symbol references. for each undefined
symbol in a .o, it finds the definition in another .o or library, then
patches the relocation entries with actual addresses.

static linking: library code copied into the binary. the .a is an
archive; the linker extracts needed .o files and includes them.
result: standalone binary, larger, no runtime library dependency.

dynamic linking: library code not copied. the binary records "I need
libfoo.so at runtime." the dynamic linker (ld-linux.so) resolves
symbols at load time. result: smaller binary, shared memory across
processes, but runtime dependency on .so presence and version.

security note: dynamic linking introduces runtime attack surface.
LD_PRELOAD injection, symbol interposition, library search path
manipulation (LD_LIBRARY_PATH, RPATH, RUNPATH). hardened builds
use full RELRO, BIND_NOW, and explicit RPATH to mitigate. for
security-critical systems, static linking or hermetic containers
eliminate the attack class entirely.

flags meaningful here: -l (link library), -L (library search path),
-Wl,... (pass options to ld), -static, -shared, -pie, -z relro,
-z now, -fsanitize (must also appear at compile time — sanitiser
runtime injection).

flags not meaningful here: -Wall, -O2, -std=c++17.
the linker does not parse C++. passing compilation flags to the
linker is noise at best, subtle breakage at worst.


---


## phase separation as principle

the flags meaningful to each phase are disjoint, with one exception:
span flags like -fsanitize must appear at both compilation and linking.
sanitisers instrument code at compile time and inject runtime libraries
at link time. omitting either half produces broken builds.

the correct model:

    CXXFLAGS        compilation flags only
    LDFLAGS         linking flags only
    span flags      explicit in both

a Makefile that passes CXXFLAGS to the linker conflates phases. it may
work by accident — the linker ignores unknown flags — but encodes a
falsehood about the build's structure. when CXXFLAGS grows a flag the
linker does not ignore, the build breaks in ways unrelated to the change.
