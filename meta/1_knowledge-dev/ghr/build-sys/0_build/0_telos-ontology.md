# build system — telos and ontology


## telos

maintain the invariant that all derived artifacts are consistent
with all their dependencies, doing the minimum work required.

every property a build system must have — correctness, incrementality,
reproducibility, parallelism — is a consequence of this.


---


## the problem

a program is a collection of source artifacts transformed in stages
into an executable. each stage: tool takes inputs, produces outputs.
outputs become inputs to the next stage.

naive approach: when anything changes, run everything.
correct, arbitrarily expensive.

intelligent approach: run only transformations whose inputs changed.
correct iff you know the complete dependency structure.

a build system maintains this knowledge.


---


## ontology

a build system is a declarative transformation engine over a DAG.

entities:

    source      leaf node. exists a priori. .cpp, .hpp, config files.
    target      derived node. produced by a rule. .o, binary, library.
    rule        function: prerequisites → target. concretely: shell commands.
    edge        target → prerequisite. "A requires B as input."

the graph must be acyclic. a cycle has no valid build order.

the build process: topological traversal of the subgraph reachable from
the requested target. execute rules for nodes that are stale.


---


## the 3 invariants

1. correctness

every artifact consistent with current state of all transitive dependencies.
a binary from mixed old/new objects is a violation. silent violations
are worst — no linker error, just wrong behaviour.

2. minimality

no rule executes unless its target is stale. over-building is not safety —
it is an error of omission in the graph, compensated by brute force.
the correct response: repair the graph.

3. reproducibility

identical inputs → identical outputs. regardless of machine, time, prior
state, undeclared environment. a build depending on undeclared inputs
(PATH, system headers, timestamps) is non-hermetic — a correctness failure
in disguise: undeclared dependencies.

Make does not enforce reproducibility. Bazel's central design goal is to
enforce it.


---


## why declarative

imperative: specify a command sequence. the order is explicit.

problem: the correct order is a function of the dependency graph.
for any non-trivial project, the human will get it wrong, fail to
maintain it, or fail to account for incremental rebuilds.


declarative: specify what depends on what. the system infers execution
order by topological sort. infers what needs rebuilding by comparing
timestamps (or hashes) against the graph.


the declarative approach is logically required for correctness at scale.
an imperative description is an implicit, error-prone manual encoding
of what the declarative system derives automatically.

deeper principle: separation of what from how. the dependency structure
is a fact about the program. the execution order is a consequence
derivable from that fact. conflating them mixes ontological levels.


---


## phase separation

a C++ build has distinct phases:

    preprocessing   .cpp + headers → translation unit (TU)
    compilation     TU → .o (machine code, symbols, relocations)
    linking         .o files, libraries → binary

the flags meaningful to each are disjoint. passing compilation flags
to the linker is an ontological error — applying a transformation to
a phase that cannot use it.

consequence: CXXFLAGS governs compilation. LDFLAGS governs linking.
flags that span both (e.g. -fsanitize) must appear in both explicitly.


---


## the dependency graph as central object

everything subordinate to one thing: the dependency graph.

the graph is not a means to an end — it is the build system's model
of the program's structure. it encodes the information-theoretic
relationships: which artifacts derive from which sources.

getting this graph right is the central problem.
making it automatic is the central engineering challenge.

Make's original design: programmer declares dependencies manually.
error-prone, expensive to maintain.

the -MMD -MP technique: delegate dependency discovery to the preprocessor
— the only entity with complete information about which headers each TU
actually includes. not a trick, but correct architecture.


---


## purity

a build system is pure when:

- every rule derivable from structure of the problem, not convention.
- every dependency actual — reflecting true information flow.
- no rule does more than its telos requires.
- the system is transparent: given the Makefile, one can reconstruct
  by reasoning what will happen for any state of the source tree.
- stable under change: adding a source, renaming a header, switching
  compilers — changes ripple correctly without manual Makefile edits.
