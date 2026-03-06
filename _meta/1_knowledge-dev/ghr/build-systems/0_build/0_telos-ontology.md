# 0. telos and ontology

## the problem, stated from nothing

a program of any non-trivial size is not a single thing.
it is a collection of source artifacts — text files containing
declarations, definitions, expressions in some formal language —
which must be transformed, in stages, into an executable artifact
that a machine can run.

each transformation is carried out by a tool: a compiler,
a linker, an assembler, a code generator. each tool takes
inputs and produces outputs. outputs of one stage become
inputs of the next.

the naive approach: when anything changes, run every
transformation from scratch. this is always correct
and arbitrarily expensive.

the intelligent approach: run only the transformations
whose inputs have changed since their outputs were last produced.
this is correct *if and only if* you know the complete
dependency structure — which transformation depends on what.

a build system is the mechanism that maintains this.

its telos, stated with precision:

    given a set of source files and a declared graph of
    transformations, produce all required derived artifacts
    in a state consistent with the current content of all
    their dependencies, doing no more work than necessary.

every property a build system must have — correctness,
incrementality, reproducibility, parallelism — is either
a direct consequence of this telos or a refinement of it
under additional constraints.


## the fundamental invariant

define: artifact A is *up-to-date* iff it was produced
*after* the last modification of every one of its direct
and transitive dependencies.

a build system's sole duty is to establish and maintain
this invariant for the requested targets.

two failure modes, dual and symmetric:

1. stale artifact: A is older than some dependency.
   the artifact is wrong. this is a correctness failure.

2. unnecessary rebuild: A is newer than all dependencies
   but is rebuilt anyway. this is a performance failure.

the ideal build system commits neither failure, ever.
this is not possible without an accurate dependency graph.

note what this means: the dependency graph is not an
optimisation concern. it is a *correctness* concern.
an inaccurate dependency graph does not merely slow
things down — it produces incorrect artifacts.
this is the deepest justification for automatic
dependency generation, which we will examine in detail later.


## what a build system *is*, ontologically

a build system is a *declarative transformation engine*
operating over a *directed acyclic graph* (DAG).

the entities:

- source: a leaf node. exists a priori, not produced
  by the build. examples: .cpp files, .hpp files,
  configuration files.

- target: a derived node. produced by executing a rule.
  examples: .o files, executables, libraries.

- rule: a function mapping a set of prerequisites
  to a target. concretely: a shell command or sequence
  of commands that, given its inputs, produces its output.

- dependency edge: a directed edge (target → prerequisite)
  stating that target's rule requires prerequisite as input.

the graph: targets and sources are nodes; dependency edges
connect them. the graph must be acyclic — a circular
dependency has no valid build order and is an error.

the build process is a *topological traversal* of this DAG:
visit nodes in dependency order (prerequisites before targets),
execute rules for nodes that are out-of-date.

this is the complete abstract model. every build system —
Make, Ninja, Bazel, Buck, Shake, Gradle — is a realisation
of this structure, differing only in:

- how the graph is declared (syntax)
- how staleness is determined (mtime, content hash, etc.)
- what the execution model is (sequential, parallel, distributed)
- how much of the graph can be inferred vs must be declared
- what invariants beyond correctness are enforced (hermeticity,
  reproducibility, caching)


## the 3 invariants a build system must honour

### 1. correctness

the produced artifacts must be consistent with the current
state of all inputs. formally: for every derived artifact A
with dependency set D(A) (all transitive dependencies),
A must have been produced in a state where every d in D(A)
had its current content.

violating this is catastrophic: you run a program built
from a mix of old and new source. the failure may be
silent — no linker error, just wrong behaviour.

### 2. minimality

no rule is executed unless its target is out-of-date.
equivalently: the build executes the *minimal* subgraph
of the DAG required to bring all requested targets
up-to-date.

this is not merely efficiency. it is also a logical
property: a rule that runs unnecessarily has no
dependency-theoretic justification. minimality is
the *correct* behaviour; over-building is an error
of omission in the dependency graph.

### 3. reproducibility (ideal)

given identical inputs, the build produces identical
outputs, regardless of: the machine, the time, the
prior state of the output directory, environment
variables not declared as inputs.

a build that depends on undeclared inputs (PATH,
system headers, timestamps, environment state) is
called *non-hermetic*. non-hermetic builds violate
reproducibility. this is a correctness failure in
disguise: the undeclared input is a real dependency
not represented in the graph.

full reproducibility is an ideal. Make does not enforce
it. Bazel's central design goal is to enforce it.
the gap between these philosophies is a deep question
in build system design.


## the declarative nature: why it is logically necessary

imperative approach: you specify a sequence of commands
to execute. the order is explicit. nothing is inferred.
rebuilding means re-running the sequence.

problem: the human specifies an order. the correct
order is a function of the dependency graph. for any
non-trivial project, the human will get this wrong,
or fail to maintain it, or fail to account for
incremental rebuilds correctly.

declarative approach: you specify *what depends on what*
and *what rule produces what*. the system infers
the correct execution order by topological sort.
it infers what needs rebuilding by comparing timestamps
(or content hashes) against the dependency graph.

the declarative approach is not merely more convenient —
it is *logically required* for correctness at scale.
an imperative build description is an implicit,
error-prone manual encoding of what the declarative
system derives automatically.

this connects to a deeper principle: the *separation of
what from how*. declaring the dependency structure is
a statement of fact about the program. the execution
order is a consequence derivable from that fact.
conflating them — encoding the order imperatively —
mixes ontological levels.

Make is declarative in this sense. a Makefile is
a set of facts about dependencies and a set of
rules for deriving targets. Make's engine reasons
over these facts to determine the minimal build plan.

we will explore the precise logical structure of this
— its connection to Horn clauses and logic programming —
in 2_declarative-functional-paradigm.md.


## what "noetically pure" means for a build system

a build system is noetically pure when:

- every rule is derivable from the structure of the problem,
  not from convention or habit.

- every dependency is *actual* — reflecting the true
  information flow of the compilation process.

- no rule does more than its telos requires. side effects,
  redundant actions, magic incantations: all eliminated.

- the whole system is *transparent*: given the Makefile,
  one can reconstruct, by pure reasoning, what will
  happen for any state of the source tree.

- the system is *stable under change*: adding a new source
  file, renaming a header, switching compilers —
  these changes ripple correctly through the system
  without requiring manual updates to the Makefile.

impurity manifests as:

- blunt prerequisites (all headers → every object)
  that approximate, rather than declare, actual dependencies.

- rules that conflate compilation and linking flags,
  encoding phase-specific concerns in phase-agnostic variables.

- duplication of rule structure across build variants,
  where a single parameterised form would express the same truth.

- scattered .PHONY declarations, inconsistent variable use,
  rules that depend on file system side effects not represented
  in the graph.

every pathology in a real Makefile is a deviation from
purity, traceable to one of these categories.


## the build phases and their logical separation

a C++ build has distinct phases, each a transformation:

1. preprocessing: resolve #include directives, macros.
   input: .cpp + headers. output: translation unit (TU).
   this phase *reveals* the true dependency graph.

2. compilation: translate TU to object code.
   input: TU (as produced by preprocessing). output: .o file.

3. linking: combine object files and libraries into
   an executable or shared library.
   input: .o files, .a/.so files. output: binary.

these phases are logically distinct. the flags meaningful
to each phase are distinct. conflating them — passing
compilation flags to the linker — is an ontological error:
you are applying a transformation to a phase that cannot
use it. it may be harmless (the linker ignores unknown
flags) or it may produce subtle errors (wrong optimisation
level, wrong ABI).

a noetically pure build system represents each phase
explicitly, uses phase-appropriate variables, and does
not bleed concerns across phase boundaries.

the practical consequence: CXXFLAGS (compilation),
LDFLAGS (linking), and flags that span both
(e.g. -fsanitize, which must be present at both
compile and link time to correctly instrument and
link the sanitiser runtime) must be declared separately
and composed explicitly.


## the dependency graph as the central object

everything in a build system is subordinate to one thing:
the dependency graph.

the graph is not a means to an end — it *is* the build
system's model of the program's structure. it encodes,
precisely, the information-theoretic relationships
between artifacts: which artifacts contain information
derived from which sources.

getting this graph right is the central problem.
making it automatic — so that the graph is always
accurate without manual maintenance — is the central
engineering challenge.

Make's original design delegates this to the programmer.
the programmer declares dependencies manually. this is
error-prone and expensive to maintain.

the `-MMD -MP` technique delegates dependency discovery
to the C preprocessor, which is the only entity
with complete information about which headers are
actually included by each translation unit.
this is not a trick. it is the correct architecture:
the entity that knows the dependencies generates
their declaration.

we will examine this mechanism in full detail in
3_make-semantics.md and 4_make-idioms-and-folklore.md.


## summary: the essential structure

a build system is:

- an engine for maintaining, correctly and minimally,
  a set of derived artifacts as their dependencies evolve.

- operating over a DAG of targets, sources, and rules.

- declarative in nature because the execution order
  is a logical consequence of the dependency structure,
  not a fact to be manually specified.

- correct only when its dependency graph is accurate.

- pure when every element of the system is derivable
  from the structure of the problem, not from habit
  or convention.

the telos: not "compile my code". the telos is:
maintain the invariant that all derived artifacts
are consistent with all their dependencies,
doing the minimum work the dependency structure requires.

this is the foundation from which everything else
in these notes is derived.