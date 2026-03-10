# staleness and correctness models


## the staleness predicate, stated formally

every incremental build system must answer 1 question
for every node in the dependency DAG:

    is this target out-of-date with respect to its inputs?

call this the staleness predicate: stale(T) → {true, false}.

a target T with prerequisite set P is stale when T must
be rebuilt to satisfy the correctness invariant. stated
formally: T is stale if and only if the artifact currently
on disk at T was not produced from the current content of
every element of P (and transitively, of every element of
their prerequisite sets).

this is the ideal predicate — defined in terms of content
identity. a build system is correct if and only if its
staleness predicate agrees with the ideal on every node.
a build system is minimal if and only if it never returns
true when the ideal predicate returns false.

the central problem of build system design is: how do you
compute this predicate efficiently without re-running every
rule on every invocation?


## mtime as a proxy

Make computes the staleness predicate by comparing
modification timestamps:

    stale_make(T) = ∃ p ∈ P : mtime(p) > mtime(T)

if any prerequisite has a modification time strictly
greater than the target's modification time, the target
is considered stale and its rule fires.

this is a proxy for the ideal predicate, not the ideal
predicate itself. mtime tracks when a file was last
written, not what content it contains. the 2 are
correlated in normal development — editing a file
updates its mtime — but the correlation is not an
identity.

the gap between proxy and ideal is the source of all
of Make's correctness limitations. it is not a bug in
Make's implementation; it is the logical consequence
of choosing mtime as the staleness signal.


## failure modes of mtime-based staleness

there are 4 distinct failure modes, 2 in each direction.


### false stale (spurious rebuild)

the proxy returns true when the ideal predicate returns
false. T is rebuilt unnecessarily.

**touch without change:**
a file is touched (mtime updated) without its content
changing. `touch foo.cpp` marks all .o files depending
on foo.cpp as stale. they recompile to identical content.
the binary is unchanged. the build was correct but not
minimal — it violated the minimality invariant.

this failure is benign in isolation: the produced
artifact is correct. it wastes time. it becomes
significant in CI/CD pipelines where build times are
critical, or when distributed caching is in use —
a spurious rebuild misses a cache hit that would have
been valid.

**rebuild to identical content:**
a rule fires and produces output byte-for-byte identical
to what was already on disk. the output's mtime is now
updated. any targets that depend on this output are
now marked stale in turn, even though nothing changed.
false staleness can cascade.

in a content-hash system this does not occur: the hash
of the output is compared to the cached hash; if they
match, downstream targets are not marked stale. Make
has no such mechanism.


### missed reuse (silent stale — catastrophic)

the proxy returns false when the ideal predicate returns
true. T is not rebuilt but should be. the artifact on
disk is inconsistent with its inputs.

**clock skew:**
on a distributed filesystem (NFS, network-mounted volume),
clocks on different machines may disagree. a file written
on machine A at its local time T₁ may have an mtime less
than T₁ as observed on machine B, if B's clock runs
behind. if B then checks staleness, it sees mtime(p) ≤
mtime(T) and concludes the target is up-to-date — even
though the prerequisite was modified after the target
was produced.

clock skew can also occur on a single machine after an
NTP correction, a VM clock adjustment, or a manual time
change. any event that moves the system clock backward
relative to file mtimes is a potential correctness
violation for any mtime-based build.

**sub-second resolution (the same-second problem):**
POSIX filesystems historically recorded mtime at 1-second
granularity. if a prerequisite and its target are both
written within the same second — as can occur on fast
machines where compilation of a small file completes in
milliseconds — the comparison mtime(p) > mtime(T) may
return false even though p was modified after T was
produced.

Linux ext4, APFS, ZFS, and most modern filesystems now
provide nanosecond mtime resolution, largely eliminating
this failure mode in practice. it is not theoretically
eliminated: it is resolved by the filesystem's clock
precision, not by any property of the build model.
on older systems, on certain virtual filesystems, and
in test environments with mocked timestamps, the failure
is reachable.

**asymmetry of consequences:**
false-stale failures are safe — the build remains
correct, only minimality is violated. missed-reuse
failures are unsafe — the build produces an incorrect
artifact while reporting success. the asymmetry is
severe: false stale wastes time; missed reuse runs
a binary built from inconsistent sources, silently,
with no indication that anything went wrong.


## content-hash staleness

the logically correct staleness predicate uses content
identity, not time:

    stale_hash(T) = hash(inputs(T)) ≠ stored_hash(T)

after each successful rule execution, the build system
records the cryptographic hash of every input. on the
next invocation, it recomputes the hash of each input
and compares. if any input's hash has changed, the target
is stale. if no input's hash has changed, the target is
up-to-date regardless of mtime.

this eliminates all 4 mtime failure modes:

- touch without change: mtime changes, hash does not.
  the target is not rebuilt.
- rebuild to identical content: the output hash matches
  the stored hash; downstream targets are not marked stale.
- clock skew: timestamps are irrelevant. only content
  determines staleness.
- same-second resolution: irrelevant. hash comparison
  has no time dimension.

the additional consequence: content-hash staleness enables
a remote build cache. if hash(inputs(T)) maps to a cached
output, that output can be retrieved without running the
rule, even on a different machine, across any time
interval. this is the foundation of Bazel's remote
execution and distributed caching. it is not achievable
with mtime: mtime encodes when, not what, and "when" is
machine-local and non-transferable.

the cost of content hashing: computing a cryptographic
hash of every input on every invocation is more expensive
than reading an mtime. for large input files (generated
code, large data dependencies), this cost is measurable.
the practical response (Bazel, Nix, Shake) is to hash
incrementally, cache hashes alongside artifacts, and
invalidate hash caches only when files change. the
overhead is acceptable at scale; the correctness
guarantee is unconditional.


## non-self-tracking: the rule-change gap

mtime-based and content-hash-based staleness both address
one question: did the inputs to this rule change? neither
addresses a second question: did the rule itself change?

a build system is self-tracking if it rebuilds targets
when the rule that produces them changes, not only when
their file inputs change. Make is not self-tracking.
most content-hash systems are.

the rule-change gap in Make: changing CXXFLAGS does not
invalidate any .o file. the .o files have file prerequisites
(the source and header files). CXXFLAGS is not a file.
Make's staleness predicate operates only over file
prerequisites. the flag change is invisible to it.

consequence: after changing CXXFLAGS without running
`make re`, the next `make` produces a binary built from
a mix of .o files compiled with the old flags and .o files
compiled with the new flags (if any sources were also
modified). the binary is incorrect and Make reports it
as up-to-date.

a self-tracking build system addresses this by including
a hash of the rule (its commands, flags, tool version)
in the staleness computation. in Shake:

    stale_shake(T) = hash(inputs(T), rule(T)) ≠ stored_hash(T)

changing the rule's flags updates rule(T), changes the
hash, and marks T stale — regardless of whether any
file input changed.

in Nix, the entire build environment is a derivation —
a formal description of every input to every build step,
including toolchain version, flags, environment variables,
and sources. a change anywhere in the derivation produces
a different hash, triggering a rebuild. this is the most
complete realisation of self-tracking.

Bazel achieves self-tracking through its action graph:
every build action is identified by a content-hash of
all its inputs, including the action itself (command,
flags, environment). changing a flag changes the action
hash, which changes the output hash, which propagates
to all downstream nodes.


## the formal classification: mokhov, mitchell, peyton jones

"build systems à la carte" (ICFP 2018) provides the most
rigorous taxonomy of build systems. it identifies 2 axes
that locate any build system in the design space:

**scheduler**: the strategy for determining which targets
to rebuild.
- topological: compute a topological ordering of the
  full dependency graph at the start; rebuild all stale
  nodes in that order. this is Make's strategy.
- restarting: start building a target; if a dependency
  is discovered to be stale mid-build, restart.
  this supports dynamic dependencies (dependencies that
  are not known until a rule runs).
- suspending: when a dependency is discovered, suspend
  the current build task, build the dependency, then
  resume. also supports dynamic dependencies, more
  efficient than restarting.

**rebuilder**: the mechanism for deciding whether a
given target is stale.
- dirty bit: a boolean flag, set when an input changes.
  resets after a successful build. Excel uses this.
  does not support minimal rebuilds across restarts
  (the dirty bit is lost if the build is interrupted).
- verifying traces: store the hashes of all inputs at
  last build. compare on next invocation. a target is
  stale if any stored hash differs from the current hash.
  this is what Shake, Bazel, and Nix implement.
  supports minimality and correctness unconditionally.
- constructive traces: store input hashes and the
  corresponding output. if the same input hashes are
  seen again, retrieve the stored output directly —
  no rule execution needed. this is the remote cache
  model: verifying traces + stored outputs = build cache.
- mtime: Make's rebuilder. not a verifying trace — no
  content identity, only temporal ordering. the source
  of all mtime failure modes described above.

Make occupies: topological scheduler + mtime rebuilder.
this combination is: fast, simple, portable, correct
under normal development conditions, and incorrect under
the failure modes enumerated above.

Shake occupies: suspending scheduler + verifying traces.
supports dynamic dependencies and is correct by
construction. the theoretical completion of what Make
approximates.

Bazel occupies: suspending/restarting scheduler +
constructive traces. adds the distributed cache.
the production-grade instantiation of the fully correct
model.


## what Make can and cannot guarantee

**can guarantee, with correct -MMD usage and
  no clock anomalies:**
- a rebuilt artifact reflects the current content of
  all declared file prerequisites.
- incremental builds do not rebuild targets whose file
  prerequisites are unchanged.
- parallel builds (-j) are correct if the dependency
  graph is complete (no undeclared dependencies).

**cannot guarantee, by design:**
- correctness after CXXFLAGS or any rule change without
  explicit `make re`. (non-self-tracking)
- correctness in the presence of clock skew. (mtime proxy)
- correctness in the presence of same-second mtime
  collisions on low-resolution filesystems. (mtime proxy)
- minimality when touch-without-change occurs. (mtime proxy)
- bit-for-bit reproducibility across machines or time.
  (non-hermetic; undeclared inputs may differ)
- correctness when undeclared file dependencies exist.
  (incomplete graph; -MMD reduces but does not eliminate
  this risk — system headers without explicit declaration
  are one remaining gap)

the guarantees Make provides are sufficient for most
single-developer C++ projects running on a single machine
with a modern filesystem. they are insufficient for:
distributed builds, shared CI caches, supply-chain-sensitive
deployments, or any context where reproducibility is a
hard requirement.

understanding these limits precisely — not as vague
"Make has problems" but as specific failure predicates —
is the prerequisite for choosing the correct tool for
a given context.


## sources

Mokhov, A., Mitchell, N., Peyton Jones, S.
"Build Systems à la Carte." ICFP 2018.
https://dl.acm.org/doi/10.1145/3236774
the definitive theoretical framework. the scheduler /
rebuilder taxonomy used above is theirs. essential reading
for anyone who wants to reason about build systems at
the level of their fundamental structure rather than
their incidental syntax.

Mitchell, N. "Shake Before Building." ICFP 2012.
https://dl.acm.org/doi/10.1145/2364527.2364538
Shake as the constructive demonstration of verifying
traces and dynamic dependencies. the concrete system
whose design informed the abstract taxonomy.

Dolstra, E. "The Purely Functional Software Deployment
Model." PhD thesis, Utrecht University, 2006.
https://edolstra.github.io/pubs/phd-thesis.pdf
Nix as the mathematically complete realisation: every
build input is a term in a purely functional language;
derivations are content-addressed. the formal treatment
of hermeticity and reproducibility.

GNU Make manual, section 4.2: Rule Syntax.
https://www.gnu.org/software/make/manual/
the specification of Make's mtime comparison semantics.
the manual states the model explicitly; the failure
modes follow by logical analysis.

Kernel.org documentation: filesystem timestamp
resolution.
https://www.kernel.org/doc/html/latest/filesystems/
relevant for understanding where the same-second
problem applies and where it has been resolved.