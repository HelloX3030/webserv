# 3. make semantics

Make's behaviour is only fully predictable when its evaluation
model is understood as a whole. Many Makefile bugs — silent
stale builds, variables expanding to the wrong thing, rules
firing in the wrong order — trace to a single misunderstanding:
Make does not evaluate a Makefile the way a shell evaluates
a script. It operates in 2 distinct phases with fundamentally
different semantics.


## the two-phase evaluation model

Phase 1 — parsing and graph construction:
Make reads the entire Makefile. It expands variables where
expansion is due (explained below), builds the complete
dependency graph, and interns all rules. No rule action
(shell command) is executed in this phase. No file is
touched. The result is a fully constructed DAG held in memory.

Phase 2 — goal-directed execution:
Make identifies the requested target (the goal), traverses
the DAG from that target, determines which nodes are stale,
and fires the rule actions for those nodes in topological
order. Rule actions are handed to a sub-shell for execution.

The implication: by the time any compilation begins, Make
already holds the complete picture of what depends on what.
This is why rules can appear in any textual order in the
Makefile — the graph is resolved structurally, not by
reading sequence. It is also why a variable used in a
rule action may expand differently than the same variable
used in a prerequisite list, depending on when each is
evaluated — the subject of the next section.


## variable assignment: 3 flavours and their logical meaning

Make has 3 primary assignment operators, each with a
distinct evaluation semantics. The distinction maps
precisely onto the eager vs lazy evaluation distinction
in programming languages.

### := simply expanded (eager)

```makefile
OBJ_DIR := obj
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
```

The right-hand side is expanded immediately, at the moment
the assignment is parsed (phase 1). The variable stores
the resulting string. Subsequent changes to variables
referenced on the right-hand side have no effect.

This is eager evaluation — the value is computed once,
at definition time. Use := whenever the right-hand side
depends on values that are fully defined at that point
and should not change. It is also the only safe choice
for variables used in shell-assignment context (!=) or
recursive functions, since = can produce infinite loops.

### = recursively expanded (lazy)

```makefile
CXXFLAGS = -Wall -Wextra $(EXTRA_FLAGS)
```

The right-hand side is stored unevaluated. Expansion
happens at each point of use — wherever $(CXXFLAGS) appears
in the Makefile. If EXTRA_FLAGS is defined after CXXFLAGS,
it will still be incorporated, because the expansion of
CXXFLAGS is deferred until use.

This is lazy evaluation — the value is computed on demand,
with the environment at the point of use. The Haskell analogy
is exact: a lazily evaluated binding in Haskell is not
computed until its value is forced; = in Make defers
expansion to the forcing point (use site).

Risk: recursive expansion can produce infinite loops if
a variable references itself, and introduces subtle
ordering dependencies — a variable's effective value
depends on the state of its referenced variables at
every point of use, not just at definition. This makes
= variables harder to reason about in large Makefiles.

Principle: prefer := unless deferred expansion is
specifically required. Deferred expansion is required
when a variable must incorporate values not yet defined
at the assignment point — common in modular Makefiles
built from included fragments.

### ?= conditional assignment

```makefile
CXX ?= c++
```

Assigns only if the variable is not already set. If CXX
is defined in the environment or by a prior assignment,
?= is a no-op. Used to establish defaults that the caller
can override without editing the Makefile — the idiomatic
way to make a build configurable from the command line:

```bash
make CXX=clang++ all
```

CXX set on the command line takes precedence over ?=
(and over = and :=, unless override is used). This is
Make's rudimentary dependency injection mechanism.

### != shell assignment

```makefile
GIT_HASH != git rev-parse --short HEAD
```

Executes the right-hand side as a shell command at parse
time (phase 1) and assigns the output. Equivalent to the
GNU Make extension $(shell ...) on the right-hand side of
:=. Use sparingly: shell invocations at parse time slow
every Make invocation, even when no rebuild is needed.


## += append

```makefile
CXXFLAGS := -Wall -Wextra
CXXFLAGS += -std=c++17
```

Appends to an existing variable, preserving a single
space separator. The semantics of += inherit the flavour
of the variable's original assignment: if CXXFLAGS was
defined with :=, += expands immediately; if defined with =,
+= defers. This inheritance is a subtlety that catches
the unwary — appending to a = variable does not make it :=.


## automatic variables: the clause instantiation mechanism

Automatic variables are bound during rule resolution —
they are the mechanism by which a rule refers to the
specific targets and prerequisites it was instantiated with.
They are only valid inside rule actions (the imperative
layer); they are not meaningful in prerequisite lists
(with the exception of secondary expansion, below).

$@  — the target of the rule.
     In a pattern rule, the fully instantiated target name.

$<  — the first prerequisite.
     In a pattern rule, the first prerequisite after
     pattern substitution. Used in compilation rules
     to refer to the source file.

$^  — all prerequisites, deduplicated, space-separated.
     Used in link rules to enumerate all object files.

$+  — all prerequisites with duplicates preserved.
     Rarely needed; useful when link order matters
     (some linkers require libraries listed multiple times
     for circular dependencies between archives).

$*  — the stem matched by % in a pattern rule.
     If the rule is `%.o: %.cpp` and the target is
     `foo/bar.o`, then $* is `foo/bar`. Used when
     constructing output paths that mirror source paths.

$(@D), $(@F)  — directory and file parts of $@.
               $(@D) for `obj/http/Request.o` is `obj/http`.
               Used in mkdir -p commands to create output
               directories matching source structure.

$(<D), $(<F)  — directory and file parts of $<. Analogous.

Example — the canonical compilation rule:

```makefile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
```

$(@D): creates the output directory structure before
       writing to it — without this, make fails if
       obj/http/ does not yet exist.
$<:    the matched .cpp file — not $^, which would
       include all prerequisites (headers too, once
       -MMD is active). Passing headers to -c would
       be a phase category error.
$@:    the target .o file — the output path.


## pattern rules: unification semantics

A pattern rule is a universally quantified rule schema:

```makefile
%.o: %.cpp
	$(CXX) -c $< -o $@
```

"For all strings S, S.o can be produced from S.cpp
by executing this action." The % is a logic variable
unified against actual target names during phase 2.

When Make needs to build `foo/bar.o` and no explicit
rule exists for it, it searches its rule database for
a pattern that matches. `%.o: %.cpp` matches with
stem S = `foo/bar`, so the prerequisite becomes
`foo/bar.cpp`. If that file exists (or can itself be
built), the rule is instantiated and added to the graph.

Chained pattern rules: Make can chain pattern rules
automatically. If `%.o: %.cpp` and `%.cpp: %.y` both
exist, Make will build `foo.o` from `foo.y` via an
intermediate `foo.cpp`. This is implicit rule chaining.
It is powerful and also a source of surprises: Make
may find a chain you did not intend. Explicit rules
always override pattern rules for specific targets —
use this to prevent unintended chaining.

Static pattern rules (a stricter form):

```makefile
$(OBJ_FILES): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

The target list (`$(OBJ_FILES)`) is explicit. Make
only applies this rule to targets in that list, not
to any .o file it encounters. This prevents the rule
from being applied where not intended — more robust
than an unconstrained pattern rule in large projects.


## the dependency file mechanism: -MMD -MP and -include

This is the most important idiom in Make for C/C++.
The logical argument for it was made in
0_telos-and-ontology.md; here is the mechanism.

Adding `-MMD -MP` to CXXFLAGS causes the compiler to
emit a `.d` file alongside each `.o` during compilation:

```
-MMD  emit a dependency file (foo.d) as a side effect
      of compilation. the file is a Make rule fragment
      recording exactly which headers were included.
-MP   emit a phony target for each header listed as
      a dependency. prevents Make from erroring if a
      header is deleted (without -MP, Make tries to
      rebuild the deleted header and fails).
```

The emitted .d file for `src/http/Request.cpp` might be:

```makefile
obj/http/Request.o: src/http/Request.cpp \
  include/http/Request.hpp \
  include/base/Logger.hpp \
  include/base/defines.hpp
include/http/Request.hpp:
include/base/Logger.hpp:
include/base/defines.hpp:
```

The first rule is an explicit dependency rule for the
object file — more precise than anything the programmer
could write by hand. The subsequent lines are the phony
targets added by -MP.

These fragments are incorporated into the Makefile via:

```makefile
DEP_FILES := $(OBJ_FILES:.o=.d)
-include $(DEP_FILES)
```

The leading `-` suppresses errors on the first build,
when no .d files exist yet (they are generated during
compilation). On subsequent builds, they are present
and loaded, giving Make precise per-TU dependency
information.

Critical interaction: once -MMD is active, the `$(H_FILES)`
blunt prerequisite on the pattern rule must be removed.
Keeping it alongside .d file inclusion means any header
change still triggers a full rebuild — the benefit of
precise dependencies is entirely nullified. The 2
mechanisms are mutually exclusive: use one or the other,
never both.

Parallel build correctness: under `make -j N`, the
precision of .d file dependencies becomes a safety
property, not merely a performance optimisation. With
blunt prerequisites, independent object files share
the same set of prerequisites (all headers) — creating
false serialisation (Make cannot parallelise rules that
share prerequisites unless it can determine they are
actually independent). With .d files, independence is
declared precisely, allowing full parallelism where it
is safe.


## .PHONY: the logical necessity

Covered in document 2, but the Make-specific mechanism
warrants precision here.

.PHONY is a declaration to Make's staleness-checking
phase: do not look for a file with this name; always
treat this target as stale. Formally, it removes a
target from the mtime-based up-to-date test.

```makefile
.PHONY: all clean fclean re debug
```

Best practice: declare all non-file targets as .PHONY.
The cost is zero. The benefit: a target named `all` or
`clean` will not silently become a no-op if someone
creates a file by that name in the project root —
a real failure mode in practice.

Group all .PHONY declarations at the top of the Makefile,
in a single directive. Scattered .PHONY declarations
across the file are functionally equivalent but harder
to audit — you cannot determine at a glance which
targets are phony without reading the entire file.


## order-only prerequisites: existence vs content

Covered in document 2 from the logical angle. The
Make-specific syntax and canonical use:

```makefile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@
```

The object file has a normal prerequisite on the source
(content dependency — changes trigger recompilation) and
an order-only prerequisite on the directory (existence
dependency — the directory must exist before the rule
runs, but changes within it do not invalidate the object).

Without the order-only prerequisite, 2 problems arise:
the compilation rule fails if the directory doesn't exist,
and adding any file to the directory marks all objects
stale (since the directory's mtime updates when its
contents change).

An alternative: use `@mkdir -p $(@D)` directly in the
rule action, removing the directory target entirely.
This is simpler for projects where all output directories
are created by the compilation rules themselves, and
avoids the need to declare directory targets explicitly.
The trade-off: `mkdir -p` is called on every compilation
invocation (cheap, but not zero cost), vs the directory
target being built once and then considered up-to-date.


## GNU Make extensions beyond POSIX

POSIX Make is a subset. GNU Make adds significant
capability. Know the boundary — code using GNU Make
extensions will not work with BSD make, nmake, or
strictly POSIX-compliant make.

Key GNU Make extensions used in real projects:

$(foreach var, list, text) — iterate over a list,
expanding text with var bound to each element.
Functionally: a map over a list.

```makefile
SUBDIRS := http config runtime
CLEAN_TARGETS := $(foreach d, $(SUBDIRS), clean-$(d))
```

$(eval text) — parse and evaluate text as Makefile
syntax at runtime. Make's macro system — roughly
equivalent to Scheme's eval, but without hygiene or
a proper type system. Powerful and dangerous: $(eval)
can generate rules, define variables, and modify the
dependency graph dynamically. Use with care; $(eval)
code is difficult to debug.

$(call func, arg1, arg2, ...) — invoke a user-defined
function. Functions are defined as variables containing
$(1), $(2), etc. as positional parameter references.
Together with $(eval), $(call) enables a limited form
of metaprogramming.

```makefile
# the rwildcard function from Lukas' Makefile:
rwildcard = $(foreach d, $(wildcard $1*), \
              $(call rwildcard, $d/, $2) \
              $(filter $(subst *, %, $2), $d))
```

This is a recursive function — possible only in GNU Make
because POSIX Make does not support $(call). It walks a
directory tree, collecting files matching a pattern.
Note: this runs at parse time (phase 1), invoking
$(wildcard) and $(filter) for every directory visited.
For deep trees, the parse overhead is measurable.

$(shell command) — execute a shell command at parse time
and substitute its output. Equivalent to != assignment.
Same caveat: incurred on every make invocation.

$(file op, filename) — read from or write to a file.
Useful for build systems that generate intermediate
data files during the build process. Available in GNU Make 4.0+.


## the include directive and Makefile composition

```makefile
include config.mk
-include $(DEP_FILES)
```

`include` inserts another file's contents into the
current Makefile at parse time. If the file does not
exist, Make attempts to build it (treating it as a
target). If it cannot be built, make errors.

`-include` suppresses the error if the file does not
exist or cannot be built. Essential for .d files on
the first build.

Makefile composition via include enables modular builds:
separate .mk fragments for flag definitions, platform
detection, test rules, etc. The Miller non-recursive
approach (document 5) relies on this — a single top-level
Makefile includes module-specific fragments, maintaining
a single dependency graph rather than splitting it across
recursive make invocations.


## parallelism: -j and its hazards

`make -j N` executes up to N independent rules concurrently.
"Independent" means no directed dependency path between them
in the DAG — they can produce their outputs simultaneously
without one needing the other's output as input.

The correctness condition: the dependency graph must be
complete. If rule B depends on the output of rule A, but
that dependency is not declared, A and B may execute
simultaneously. B reads A's output file mid-write, or
before it exists, and produces a corrupt or missing artifact.
This failure may be intermittent — it depends on scheduling —
making it one of the hardest bugs to diagnose.

With full -MMD dependency generation and no blunt
prerequisites, the dependency graph approaches completeness
and `make -j` is safe. With blunt or missing prerequisites,
`make -j` is a latent correctness hazard.

The interaction with Make's non-self-tracking limitation
is also sharp under parallelism: if you change CXXFLAGS
and run `make -j` without a prior `make re`, you may get
a binary built from a mix of old and new compilation flags,
with no indication anything went wrong.

Recommended practice for the projects in this knowledge
base: always use `make -j$(nproc)` for speed, but treat
any build that touches CXXFLAGS, LDFLAGS, or structural
Makefile changes as requiring `make re` first.


## language comparisons

Haskell — lazy vs eager evaluation:
The = vs := distinction in Make maps directly onto
Haskell's evaluation model. In Haskell, all bindings
are lazy by default — a value is computed only when
forced. `let x = expensive` in Haskell is equivalent
to Make's `x = expensive`: the right-hand side is
not computed until x is used. Strict evaluation in
Haskell (via `seq`, `$!`, or `BangPatterns`) corresponds
to :=: compute now, store the result. The Haskell
compiler is explicit about this distinction in a way
Make is not — another case where Make's informality
is a source of subtle bugs.

Shake (Haskell build system):
Shake, written by Neil Mitchell, is a build system
embedded in Haskell. Rules are Haskell functions.
Dependencies are dynamic — a rule can discover
additional dependencies at runtime by calling `need`.
This resolves Make's static-dependency limitation
entirely: the dependency graph is a first-class Haskell
value, constructed by arbitrary Haskell code.
Shake is the existence proof that the functional
analogy is not merely poetic — a build system can
literally be a pure functional program.
Reference: Mitchell, N. "Shake Before Building." ICFP 2012.
https://dl.acm.org/doi/10.1145/2364527.2364538

Rust — build.rs:
Rust's build system (Cargo) supports a `build.rs` file —
a Rust program executed at build time to generate code,
discover system libraries, and emit dependency information
via `println!("cargo:rerun-if-changed=...")`. This is
dynamic dependency declaration in a strongly typed
language — conceptually similar to Shake's `need`, but
more constrained. The `rerun-if-changed` directive is
Cargo's equivalent of a .d file: it tells the build
system exactly which files, if changed, should trigger
re-execution of the build script.

Guile Scheme — eval as a model:
Make's $(eval) is a crude, untyped version of Scheme's
eval. In Guile Scheme, eval takes an s-expression and
evaluates it in a given environment — the foundation
of Scheme's homoiconicity (code and data have the same
representation). Make's $(eval) takes a string of
Makefile syntax and parses it — a much weaker form,
with no hygiene guarantees, no lexical scope, and no
type system. The structural idea is the same: the build
system generates and evaluates its own rules at runtime.
Guix, the GNU package manager built on Guile, takes
this to its logical extreme: the entire build system is
a Guile Scheme program, and every package definition is
a Scheme value. This is the fully realised version of
what Make's $(eval) gestures toward.
Reference: Courtès, L. "Functional Package Management
with Guix." European Lisp Symposium 2013.
https://arxiv.org/abs/1305.4584


## sources

GNU Make manual. https://www.gnu.org/software/make/manual/
Chapters 3-6 (writing rules, variables, conditionals)
and chapter 10 (implicit rules) cover all mechanisms
described here. The authoritative reference.

POSIX make specification.
https://pubs.opengroup.org/onlinepubs/9699919799/utilities/make.html
The baseline standard. Identifies which features are
portable vs GNU-specific.

Mitchell, N. "Shake Before Building: Replacing Make
with Haskell." ICFP 2012.
https://dl.acm.org/doi/10.1145/2364527.2364538
The most complete critique of Make's static-dependency
and non-self-tracking limitations, with Shake as the
constructive response. Read alongside the Mokhov et al.
theoretical framework.

Courtès, L. "Functional Package Management with Guix."
European Lisp Symposium 2013.
https://arxiv.org/abs/1305.4584
The fully realised functional build system — build
descriptions as first-class values in a homoiconic language.

Miller, P. "Recursive Make Considered Harmful." 1997.
https://aegis.sourceforge.net/auug97.pdf
The foundational critique of recursive Make. Context
for the include-based composition described here and
developed further in 5_build-variant-architectures.md.