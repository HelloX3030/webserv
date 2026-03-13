# sources


## foundational

Feldman, S. "Make — A Program for Maintaining Computer Programs." 1979.
    the original. short paper, still worth reading for design intent.

Stallman, R. et al. GNU Make Manual.
    https://www.gnu.org/software/make/manual/
    the authoritative reference. dense but complete.

Mokhov, Mitchell, Peyton Jones. "Build Systems à la Carte." ICFP 2018.
    the definitive formalisation. classifies build systems along
    static/dynamic and minimal/cutoff axes. derives Shake, Bazel, Make
    as instances of a general framework.

Miller, P. "Recursive Make Considered Harmful." 1998.
    why naive recursive Make breaks the dependency graph.


---


## tools and systems

GNU Make
    https://www.gnu.org/software/make/

Ninja
    https://ninja-build.org/
    generated build files, not hand-written. fast.

Bazel
    https://bazel.build/
    hermetic, reproducible, content-addressed. Google's internal build.

Buck / Buck2
    https://buck.build/ (Meta)
    similar design space to Bazel.

Shake
    https://shakebuild.com/
    Haskell-embedded DSL. dynamic dependencies, monadic rules.

Tup
    http://gittup.org/tup/
    filesystem monitoring, fully automatic dependency detection.

Redo (djb)
    https://cr.yp.to/redo.html
    minimal, shell-based, dynamic dependencies.

Nix
    https://nixos.org/
    functional package manager. hermetic by construction.
    builds are pure functions from inputs to outputs.

Guix
    https://guix.gnu.org/
    Nix model, Scheme-based. full system reproducibility.

Meson
    https://mesonbuild.com/
    generates Ninja. simpler than CMake, correctness-oriented.

CMake
    https://cmake.org/
    meta-build system. generates Make/Ninja/etc. ubiquitous, complex.


---


## compilers and toolchains

GCC Manual — Preprocessor Options
    https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html
    -MMD, -MP, -MF, -MT documented here.

Clang Documentation
    https://clang.llvm.org/docs/
    compatible with GCC flags. additional static analysis.

LLVM
    https://llvm.org/
    compiler infrastructure. understand the pipeline: frontend,
    optimiser, backend.

Binutils (ld, ar, nm, objdump)
    https://www.gnu.org/software/binutils/
    linker and binary utilities. essential for understanding
    object files, archives, symbol resolution.


---


## security and supply chain

SLSA (Supply-chain Levels for Software Artifacts)
    https://slsa.dev/
    framework for supply-chain integrity. levels 1-4.
    level 3+ requires hermetic, reproducible builds.

Reproducible Builds
    https://reproducible-builds.org/
    cross-project initiative. techniques, tools, documentation.

Wheeler, D. "Fully Countering Trusting Trust through Diverse Double-Compiling."
    defense against compiler backdoors. relevant to build integrity.

Thompson, K. "Reflections on Trusting Trust." 1984.
    the original compiler backdoor thought experiment.
    why you cannot trust binaries you did not build from source
    you inspected with tools you trust.


---


## theory and background

Brooks, F. "No Silver Bullet: Essence and Accident in Software Engineering." 1987.
    essential vs incidental complexity. declarative build systems
    eliminate incidental complexity of manual ordering.

Datalog and Deductive Databases
    build systems as fixpoint computations over relations.
    Mokhov et al. formalise this connection.

Graph Theory — DAGs, Topological Sort
    Cormen et al. Introduction to Algorithms.
    the dependency graph is a DAG; the build order is a topological sort.


---


## historical

Kernighan, Pike. The Unix Programming Environment. 1984.
    Make in context. philosophy of small tools, pipelines.

Oram, Talbott. Managing Projects with GNU Make. O'Reilly, 3rd ed.
    practical guide. some patterns dated, fundamentals solid.

Recursive Make Considered Harmful (Miller)
    already listed above. historically significant.


---


## advanced / research

Esfahani et al. "CloudBuild: Microsoft's Distributed and Caching Build Service." 2016.
    industry-scale distributed builds.

Google. "Bazel: Correct, Reproducible, Fast — Choose Three."
    design rationale for hermeticity and content-addressed caching.

Mitchell, N. "Shake Before Building." ICFP 2012.
    Shake's design. dynamic dependencies done right.

Dolstra, E. "The Purely Functional Software Deployment Model." PhD thesis, 2006.
    Nix's theoretical foundation. builds as pure functions.
