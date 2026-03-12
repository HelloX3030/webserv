# build variant ontology


## essence

a build variant is a realisation of the same source tree under
a different configuration of the transformation pipeline.
source files are identical across variants; what varies is the
parametrisation of compilation and linking.


---


## the 4 variant dimensions

exactly 4 things vary between any 2 variants:

compilation flags: additions to base CXXFLAGS for this variant.
examples: -DDEBUG=1, -g, -O0, -fsanitize=address.
these govern .o production.

link flags: additions to base LDFLAGS for this variant.
examples: -fsanitize=address (runtime injection), -lgcov (coverage).

object directory: where .o and .d files land.
variants must have separate objdirs — if release and debug share
one, a debug recompilation overwrites release objects while leaving
them timestamped newer than sources. the release binary becomes
silently inconsistent.

binary name: the output executable.
variants must have separate names to coexist on disk.

everything else — source files, include paths, base flags,
dependency graph structure, pattern rules — is invariant.
polluting the invariant layer with variant-specific concerns
is the root pathology of ad-hoc multi-variant Makefiles.


---


## the sanitiser correctness constraint

sanitiser flags (-fsanitize=address, -fsanitize=undefined, etc.)
must appear at both compilation and link time.

they are not merely compilation flags. the linker must inject
the sanitiser runtime library, which requires -fsanitize in
the link invocation.

consequence: a single CXXFLAGS variable cannot carry sanitiser
flags if used only for compilation. if CXXFLAGS is also passed
to the link step (the phase-conflation error), the sanitiser
build happens to work — but for the wrong structural reason,
and breaks when the conflation is corrected.

correct model: CXXFLAGS governs compilation, LDFLAGS governs
linking, dedicated variables (EXTRA_CFLAGS, EXTRA_LDFLAGS)
carry variant-specific additions into each phase explicitly.
sanitiser flags appear in both.


---


## architecture 1: separate files

one Makefile per variant; all shared logic duplicated in full.

this is not an architecture — it is an absence of one.
its only virtue is independent readability. every change to
shared logic (source list, include paths, base flags) must be
replicated across all files. an omitted replication is a
silent divergence.

mentioned for completeness; not used.
