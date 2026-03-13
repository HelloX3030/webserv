# dependency graph — formal model


## the graph

let S be the set of all artifacts: sources (leaf nodes) and derived
targets (internal nodes).

define the dependency relation R ⊆ S × S where (A, B) ∈ R means
"A directly depends on B" — A cannot be brought up-to-date without
first considering B.

the induced graph G = (S, R) must be a DAG. a cycle has no valid
topological ordering, no consistent notion of "up-to-date". a cyclic
dependency is not merely inconvenient — it is undefined.

entities:

    source      leaf node. exists a priori, not produced by the build.
                .cpp, .hpp, configuration files.

    target      derived node. produced by executing a rule.
                .o files, executables, libraries.

    rule        function: prerequisites → target.
                concretely: shell commands that, given inputs, produce output.

    edge        directed: target → prerequisite.
                "this target's rule requires this prerequisite as input."


---


## the build process

a restricted topological traversal. for requested target T:

1. compute the subgraph reachable from T under R
2. topologically sort it
3. for each node in that order: execute its rule iff out-of-date
   with respect to its immediate predecessors

the restriction — only the reachable subgraph — is the minimality
invariant made operational. we do not touch what T does not need.


---


## the up-to-date invariant

artifact A is up-to-date iff it was produced after the last modification
of every one of its direct and transitive dependencies.

a build system's sole duty: establish and maintain this invariant for
the requested targets.

two failure modes, dual and symmetric:

    stale artifact          A older than some dependency.
                            the artifact is wrong. correctness failure.

    unnecessary rebuild     A newer than all dependencies, rebuilt anyway.
                            performance failure.

the ideal build system commits neither, ever. this is not possible
without an accurate dependency graph.

the dependency graph is not an optimisation concern. it is a correctness
concern. an inaccurate graph does not merely slow things down — it
produces incorrect artifacts. this is the deepest justification for
automatic dependency generation.


---


## graph completeness

two requirements:

    complete        every actual dependency is declared. if a .cpp includes
                    a header, that header is an edge in the graph. if a flag
                    changes semantics of every TU, that flag change must
                    invalidate all affected .o files.

    accurate        no edge declared that is not an actual dependency.
                    a blunt prerequisite (all headers → every .o) over-declares.
                    it enforces a constraint stronger than truth. produces
                    correct builds at the cost of minimality — a trade-off
                    never necessary when -MMD generates the exact graph.

from these: use -MMD -MP always, without exception. the only mechanism
in GNU Make that produces a complete, accurate, per-TU dependency graph.
cost: one .d file per object. there is no argument against it.


---


## the complete graph for a C++ project

for n source files and m headers:

    [.cpp₁, .hpp₁, ..., .hppₖ]  →  .o₁  ─┐
    [.cpp₂, .hpp₁, .hppⱼ]       →  .o₂  ─┤
    ...                                   ├→  [.a₁, .oₙ, ...]  →  binary
    [.cppₙ, .hppₘ]              →  .oₙ  ─┘

each .o depends on exactly the headers transitively included by its
corresponding .cpp. this set is unique per TU. only the preprocessor
can determine it. the build system's job: maintain this graph accurately
as headers are added, removed, or modified.
