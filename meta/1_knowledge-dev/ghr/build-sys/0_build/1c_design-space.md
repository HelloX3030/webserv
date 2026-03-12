# build system design space


## orientation

build systems differ along several axes. understanding the design space
clarifies what Make is, what it is not, and what trade-offs it embeds.
this is not academic taxonomy — it determines what guarantees you have
and what failure modes you inherit.


---


## axis 1: dependency declaration

### explicit

the programmer declares all dependencies. Make without -MMD, plain shell
scripts. risk: human error. a missing dependency means a stale build goes
undetected. the artifact is wrong; the build system reports success.

### compiler-assisted

the compiler emits dependency information as a side effect of compilation.
Make with -MMD -MP. the dependency graph approaches correctness — accurate
for headers, still manual for source file lists (mitigated by $(wildcard)).

### fully automatic

the build system tracks all file accesses by all tools via filesystem hooks,
strace, or similar. any file read by any tool is automatically a dependency.
examples: Tup, Redo (djb), certain Bazel configurations.

this is the logically complete solution. no dependency can be missed because
observation is exhaustive. the trade-off: implementation complexity, platform
dependence, performance overhead of interception.


---


## axis 2: staleness detection

### mtime-based

a target is stale if its mtime is older than any prerequisite's mtime.
Make uses this model. mtime is a proxy for content change — imperfect.

failure modes:

    false stale     file touched without content change. unnecessary rebuild.
    missed reuse    file rebuilt to identical content. downstream rebuilds
                    that could have been skipped.
    clock skew      distributed builds across machines with unsynchronised
                    clocks. a prerequisite in the future makes the target
                    perpetually stale.

mtime is fast to check (single stat call) but semantically imprecise.

### content-hash-based

a target is stale if the content hash of any input has changed since the
target was produced. examples: Bazel, Shake, Nix.

this is the logically correct model. consequence: identical inputs →
cached output, always. enables distributed caching — if another machine
already built this exact input set, fetch the result instead of rebuilding.
enables true reproducibility — the hash is a content address.

trade-off: computing hashes is more expensive than stat. for large files
or many files, the overhead is measurable. modern systems amortise this
via incremental hashing, caching hash results, and parallelism.


---


## axis 3: hermeticity

### open (Make)

the build can access anything on the filesystem: system headers, environment
variables, external tools, the current working directory, the network.

fast to set up. non-reproducible in general. a build that works on your
machine may fail elsewhere because it depends on undeclared system state.
"works for me" is the symptom; undeclared dependencies are the disease.

security implication: an open build is a supply-chain vulnerability. any
file accessible to the build process is an implicit input. a compromised
system header, a malicious tool in PATH, an environment variable override
— all silently contaminate the output. the build system cannot distinguish
legitimate inputs from attacker-controlled ones because it does not track
what the legitimate inputs are.

### hermetic (Bazel, Nix)

every input to every rule must be declared. undeclared inputs are
inaccessible — enforced by sandbox, chroot, or container. the build runs
in an environment containing only what it explicitly requested.

consequence: the build is reproducible bit-for-bit across machines and time.
given the same declared inputs, the same output. always.

this is the gold standard for security and CI/CD. a hermetic build cannot
be subverted by ambient system state because ambient state does not exist
inside the sandbox. supply-chain attacks must compromise a declared input,
which can be pinned by hash and audited.

trade-off: setup cost. every tool, every header, every library must be
declared. bootstrapping a hermetic build from nothing is substantial work.
Nix and Guix solve this by providing a complete, hash-addressed package
universe from which inputs are drawn.

reference: SLSA framework (slsa.dev) defines supply-chain security levels.
level 3+ requires hermetic, reproducible builds.


---


## axis 4: static vs dynamic dependencies

### static

the dependency graph is fully known before any rule fires. the build system
reads the Makefile (or equivalent), constructs the complete graph, then
executes. Make and Ninja are static.

limitation: cannot express "the dependencies of A depend on the content of B."
example: a code generator reads a schema file and produces .cpp files. which
.cpp files? depends on the schema's content. a static build system cannot
know until the generator runs.

Make's -MMD approximates dynamic dependencies: the compiler discovers header
dependencies during compilation and emits them for the next build. but the
first build of a new file has no .d file — it runs with incomplete dependency
information. this is safe only because a new file has no prior .o to be stale.

### dynamic

a rule may discover additional dependencies at runtime, depending on the
content of its inputs. the build system re-evaluates the graph as execution
proceeds. examples: Shake, Bazel (with aspects), Tup.

power: can model any dependency structure, including generated code, computed
includes, and configuration-dependent compilation. the graph is a fixpoint,
not a static declaration.

trade-off: complexity. the build engine must handle graph changes mid-build.
parallelism becomes harder — a rule may add dependencies to a target another
thread is building. Shake's design (monadic build rules) handles this cleanly.


---


## axis 5: self-tracking

### non-self-tracking (Make)

the build system does not track changes to build rules themselves. modify a
rule — change CXXFLAGS, alter a recipe — and affected targets do not rebuild
automatically. the programmer must know to `make clean` or `make re`.

this is a correctness hole. CXXFLAGS is an input to every compilation; a
change to it should invalidate all .o files. Make does not model this.
the flag string is not an edge in the dependency graph.

mitigation: discipline. after any flag change, `make re`. encode this in
documentation and habit. it is manual compensation for a design limitation.

### self-tracking (Shake, Bazel)

the build system records the rules used to produce each target. if a rule
changes, targets built by that rule are stale. CXXFLAGS is part of the build
signature; changing it invalidates affected outputs automatically.

this is the correct model. the rule is an input; inputs that change trigger
rebuilds. the build signature includes: prerequisites, rule body, flags,
tool versions — everything that affects the output.

trade-off: storage. the build system must persist enough information to
detect rule changes. Shake stores a database of build history. Bazel uses
content-addressed storage with action digests.


---


## where Make sits

Make is: explicit dependencies (compiler-assisted with -MMD), mtime-based
staleness, open (non-hermetic), static dependencies, non-self-tracking.

this is not a criticism. Make is 48 years old, runs everywhere, requires
no runtime, has no dependencies of its own. for small-to-medium projects
with disciplined programmers, it is sufficient.

but know its limits. when you need reproducibility, hermeticity, or builds
that scale across teams and machines — you have outgrown Make. Bazel, Buck,
Nix, Shake exist because Make's design space position is insufficient for
those requirements.

for webserv: Make is appropriate. the project is small, single-machine,
single-developer (or small team). the discipline required — `make re` after
flag changes, -MMD for header dependencies — is manageable. understand the
limits; do not pretend they do not exist.
