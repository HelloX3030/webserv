# 2. the declarative-functional paradigm

Understanding Make's computational paradigm tells you
what it can and cannot express naturally, where its
pathologies come from, how to read and write rules
with full clarity, and how Make relates to other
systems — Datalog, Prolog, Haskell's lazy evaluation.
These are structural, not aesthetic considerations.


## imperative vs declarative

Imperative computation: a sequence of commands, each
mutating state. The programmer specifies how to reach
a desired state, step by step. Order is explicit and
essential — swap 2 lines and the result changes or breaks.

```bash
compile foo.cpp
compile bar.cpp
link foo.o bar.o
```

This encodes how. Its correctness depends on knowing
the right order in advance — which is itself a function
of the dependency structure. The programmer is doing
manually what a machine could derive automatically.

Declarative computation: a set of facts and rules
describing what is true. The system infers the correct
procedure to establish the desired state. Evaluation
order is not specified — it is derived from the logical
structure of the facts.

```makefile
binary: foo.o bar.o
foo.o: foo.cpp foo.hpp
bar.o: bar.cpp bar.hpp foo.hpp
```

This encodes what. The execution order is a consequence
of these facts, derived by topological sort. "Compile
foo before bar before linking" is not stated — it
follows necessarily from the declared structure.

The distinction is ontological, not syntactic. 
Imperative programs encode a procedure; 
declarative programs encode a relation. 
Executing a declarative program is searching for a solution 
to a constraint system, not running a recipe.


## Make rules as logical implications

Consider:

```makefile
foo.o: foo.cpp foo.hpp
	$(CXX) $(CXXFLAGS) -c foo.cpp -o foo.o
```

Read this not as an instruction but as a logical
statement: "foo.o is up-to-date if foo.cpp is
up-to-date and foo.hpp is up-to-date and the rule
has been executed with their current versions."

This is a Horn clause — a restricted form of
first-order logical implication:

    head :- body₁, body₂, ..., bodyₙ.

meaning: head is provable if all body conditions
are provable. In Prolog notation:

    up_to_date('foo.o') :-
        up_to_date('foo.cpp'),
        up_to_date('foo.hpp'),
        execute_rule('foo.o').

The build process is goal-directed resolution. Given
the goal "make T up-to-date", expand it recursively:
T is up-to-date if its prerequisites are up-to-date
and its rule has run. Each prerequisite is itself a
goal. Recursion terminates at source files — leaf nodes
of the DAG — which are axiomatically up-to-date or
declared stale based on mtime.

This is backward chaining — exactly Prolog's evaluation
strategy. Start from the goal (the requested target),
work backward through the dependency graph to determine
what must be done.


Treating a Makefile as a logic program immediately
explains several behaviours that otherwise appear arbitrary:

    Rules run in dependency order, not textual order,
    because the order of clauses in a logic program does
    not determine execution order — dependency structure does.

    A rule can appear "above" its prerequisites in the file 
    and still work: the engine resolves a goal graph, not a script.

    Circular dependencies are errors because a circular
    Horn clause set has no finite proof — the inference
    engine loops, or Make detects and rejects it.


## the Datalog connection

Datalog is a declarative query language — a restriction
of Prolog without function symbols, with guaranteed
termination. It is the natural language for fixed-point
computations over finite relations.

A build system maps directly onto a Datalog program:

The extensional database (EDB): base facts given a
priori — source file existence, mtimes, content.
Not derived.

The intensional database (IDB): derived facts — which
targets are stale, which rules must fire, which targets
are up-to-date. Computed by the rules.

The rules: Datalog clauses mapping EDB facts to IDB
facts. In Make: the dependency rules.

The query: which derived fact are we establishing?
In Make: is target T up-to-date?

Evaluating a Datalog program computes the minimal
model — the smallest set of facts satisfying all
rules given the base facts. This corresponds exactly
to Make's minimality invariant: do the minimal work
required to establish the target's up-to-dateness.

Mokhov, Mitchell, and Peyton Jones (2018) formalise
this connection rigorously, classifying build systems
along 2 axes derived from this logical model.

Static vs dynamic dependencies:
- static: the dependency graph is fully known before
  any rule fires. Make and Ninja are static.
- dynamic: a rule may discover additional dependencies
  at runtime, depending on the content of its inputs.
  Shake and Bazel support this.

Make has static dependencies. It cannot express "the
dependencies of A depend on the content of B." The
-MMD technique approximates dynamic dependencies by
running the preprocessor as a side effect of compilation
— examined in 3_make-semantics.md.

Self-tracking vs non-self-tracking:
- self-tracking: the build system tracks changes to
  build rules themselves. Change a rule, affected
  targets rebuild.
- non-self-tracking: changing a rule does not trigger
  rebuilds. Make is non-self-tracking.

If you change a flag in CXXFLAGS, Make does not know
the .o files are stale. They were produced by a
different rule, but Make sees only mtime. This is a
genuine correctness gap. The `make re` idiom exists
precisely to work around it. Proper remedies (stamp
files, rule hashing) are covered in
4_make-idioms-and-folklore.md.

Reference: Mokhov, Mitchell, Peyton Jones.
"Build Systems à la Carte." ICFP 2018.
https://dl.acm.org/doi/10.1145/3236774
The most complete theoretical map of the build system
design space. Essential reading.


## the functional analogy

View each rule as a pure function:

    rule_foo_o : (foo.cpp, foo.hpp) → foo.o

Given the same inputs, it always produces the same
output. No side effects affecting other rules. The
entire build is then a composition of pure functions —
a large expression in a functional language whose
evaluation produces the requested targets.

This analogy has precise technical content.

Memoisation: a memoised function caches its output
keyed on its inputs. Called again with the same inputs,
it returns the cached result without re-executing.
A build system is exactly memoised computation: the
.o file is the cached result of compiling its sources.
If the sources haven't changed, the cache is valid and
compilation is skipped. Staleness detection is cache
invalidation.

Lazy evaluation: in a lazily evaluated language
(Haskell), an expression is evaluated only when its
value is needed. In a build: a target is built only
if something else needs it — if it is reachable from
the requested goal. Make's goal-directed evaluation
is lazy in this sense.

Referential transparency: a pure function can be
replaced by its output wherever that output appears.
In a content-hash build system (Bazel, Nix), this is
literally true: any target with known inputs can be
replaced by its precomputed output from a cache,
without re-running the rule. Mtime-based systems like
Make approximate this but cannot guarantee it — the
mtime is a proxy for identity, not identity itself.

The functor view: at the highest level of abstraction,
a build system is a functor from the category of
source file states to the category of derived artifact
states. Each rule is a morphism; composition of rules
is functor composition. The correctness requirement
is coherence — the result of applying a sequence of
morphisms must not depend on the order of independent
morphisms (where independence means no dependency edge
between them). This coherence condition is exactly
what is violated by race conditions under `make -j`
with an incomplete dependency graph.

Reference: Mokhov. "Algebraic Graphs with Class."
Haskell 2017.
https://dl.acm.org/doi/10.1145/3122955.3122956


## Make as a hybrid: declarative structure, imperative actions

Make is not purely declarative. It is a hybrid.

The declarative layer: the dependency graph — targets,
prerequisites, their relationships. The engine reasons
over this layer to determine the build plan.

The imperative layer: the rule actions — the shell
commands in each rule. Sequential imperative scripts
that mutate the filesystem. Not evaluated by Make's
logic engine; handed to a shell for execution.

This hybrid is both Make's strength and the source
of its pathologies.

Strength: the imperative layer is maximally flexible.
Any shell command can appear in a rule.

Pathology 1: a rule that writes to files not declared
as outputs creates undeclared side effects — invisible
to the dependency graph. Subsequent rules depending on
those files have no declared edge and may run in the
wrong order.

Pathology 2: if a rule's commands change (e.g. different
flags), Make does not know the outputs are stale. The
declarative layer tracks input file changes, not rule
changes. Non-self-tracking, as above.

Pathology 3: order within the imperative layer is
fully manual. The sequence of shell commands in a rule
is an imperative script — its correctness is the
programmer's responsibility entirely.

The art of writing a good Makefile is maintaining the
purity of the declarative layer whilst containing the
necessary impurity of the imperative layer.


## the logical reading of Make's special constructs

With the logical model established, Make's constructs
become transparent.

.PHONY: in the logical model, staleness is checked by
comparing mtime against prerequisites. A phony target
should always be considered stale — its rule always
fires when requested. Declaring a target .PHONY removes
it from the mtime-based staleness computation.
Logically: an axiom stating "this goal is always
unsatisfied; always re-derive it." Without .PHONY, a
target named `clean` would be considered up-to-date if
a file named `clean` exists — Make finds the fact
"true" via the filesystem and skips the rule.
.PHONY prevents this category error.

Order-only prerequisites (|):
`target: normal_prereqs | order_only_prereqs`
Order-only prerequisites must exist and be built before
the target, but changes to them do not mark the target
stale. Canonical use: directory creation.

```makefile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
```

The object file depends on the source file (content —
changes trigger recompilation) and on the directory's
existence (not its mtime — files added to the directory
do not recompile all objects). Logically: order-only
prerequisites are prerequisites on existence, not
on content.

Pattern rules (%.o: %.cpp): universally quantified
Horn clauses. "For all X, X.o is up-to-date if X.cpp
is up-to-date and the compilation rule has run."
The % is a logic variable, bound when Make unifies a
target against the pattern — Prolog-style unification.

Automatic variables ($@, $<, $^, $*): logical variables
bound during rule resolution. $@ binds to the target
(the clause head). $< binds to the first prerequisite.
$^ binds to all prerequisites. $* binds to the stem
matched by % in a pattern rule. They are the mechanism
by which rule clauses refer to the terms they were
instantiated with.


## why declarative is logically necessary

Given a project with n source files and an arbitrary
dependency graph, the question "which artifacts must
rebuild given that file F changed?" is a graph
reachability question: which targets are reachable from
F via the inverse dependency relation? Its answer is a
function of the graph structure alone.

An imperative build script cannot answer this
efficiently. It either rebuilds everything (correct,
maximally expensive) or encodes a hardwired partial
order (fragile, becomes wrong as the project evolves).

A declarative build system answers it optimally by
construction: the dependency graph is the system's
data structure, and the engine traverses it to
determine the minimal rebuild set.

The deeper principle: the form of a solution should
be isomorphic to the structure of the problem. Where
the problem is relational and logical, the solution
is declarative. Using an imperative approach for a
structurally declarative problem is a mismatch of
language to domain — harder to write, harder to
maintain, and wrong in ways that are difficult to
detect. Incidental complexity, in Brooks' sense.

Reference: Brooks. "No Silver Bullet: Essence and
Accident in Software Engineering." IEEE Computer, 1987.


## sources

Lloyd, J.W. "Foundations of Logic Programming."
Springer-Verlag, 1987 (2nd ed.). Rigorous mathematical
foundation of Horn clause logic, SLD resolution, and
Prolog semantics.

Apt, K. and van Emden, M.H. "Contributions to the
Theory of Logic Programming." JACM 29(3), 1982.
Foundational treatment of Horn clause program semantics
and minimal Herbrand models.

Mokhov, Mitchell, Peyton Jones. "Build Systems à la
Carte." ICFP 2018.
https://dl.acm.org/doi/10.1145/3236774

Mokhov. "Algebraic Graphs with Class." Haskell 2017.
https://dl.acm.org/doi/10.1145/3122955.3122956

Van Roy, P. and Haridi, S. "Concepts, Techniques, and
Models of Computer Programming." MIT Press, 2004.
Chapter 9 on relational/logic programming.
Freely available: https://www.info.ucl.ac.be/~pvr/book.html

Brooks. "No Silver Bullet." IEEE Computer, 1987.

Bernstein, D.J. "Redo." (~2010, unpublished design
notes.) Corrects Make's non-self-tracking and static-
dependency limitations by following the logical model
more faithfully. Reading its design reveals by contrast
exactly where Make deviates from purity.
Apenwarr's implementation: https://github.com/apenwarr/redo
Note: djb's original notes were never formally
published; the apenwarr implementation is the most
accessible entry point.