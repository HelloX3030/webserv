# elite makefile principles


## orientation

this document does not introduce new mechanisms. everything
described here has been established across documents 0-5.
what this document does is elevate: extract the invariants,
name the principles, and state them as the checklist against
which any Makefile can be judged.

a Makefile is not a script, but a formal declaration of
the dependency structure of a software system. therefore is held
to high standards.


## the 3 invariants (from 0_telos-ontology.md)

a Makefile that violates any of them is incorrect.

**invariant 1 — correctness.**
every produced artifact must be consistent with the current
state of every input in its transitive dependency set. a binary
built from a mix of current and stale objects is a violation.
silent violations are the worst kind.

**invariant 2 — minimality.**
no rule executes unless its target is genuinely stale. over-building
is not a safety measure — it is an error of omission in the
dependency graph, compensated by brute force. the correct response
to over-building is to repair the graph, not to accept the waste.

**invariant 3 — structural stability.**
adding, removing, or renaming a source file must not require
manual edits to the Makefile. a build system that requires the
programmer to maintain a manually curated file list is already
broken — it has externalised a derivable fact into a human process.
$(wildcard) or equivalent is not optional; it is required.


## principle 1: graph completeness is the only correctness criterion

every other principle in this document is in service of one
thing: the dependency graph must be complete and accurate.

complete: every actual dependency is declared. if a .cpp file
includes a header, that header is an edge in the graph.
if a compilation flag changes the semantics of every TU,
that flag change must invalidate all affected .o files.

accurate: no edge declared that is not an actual dependency.
a blunt prerequisite (all headers → every .o) over-declares.
it enforces a constraint stronger than truth. it produces
correct builds at the cost of minimality — a trade-off that
is never necessary when -MMD generates the exact graph.

from these 2 requirements: use -MMD -MP always, without
exception. it is the only mechanism in GNU Make that produces
a complete, accurate, per-TU dependency graph at the cost of
one extra file per object. there is no argument against it.


## principle 2: phase separation is ontological, not stylistic

compilation and linking are distinct phases of a distinct
character. the flags meaningful to each are disjoint, with
the sole exception of span flags (e.g. -fsanitize) that must
be explicit at both.

the consequence: 3 distinct flag variables, each carrying
exactly its phase's concerns:

```makefile
CXXFLAGS   := -Wall -Wextra -Werror -std=c++17  # compilation only
LDFLAGS    :=                                    # linking only
SAN_FLAGS  := -fsanitize=address -g              # both phases, explicit
```

the rule then composes them explicitly:

```makefile
$(BIN): $(OBJS)
	$(CXX) $(LDFLAGS) $(ASAN_LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(ASAN_CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
```

passing CXXFLAGS to the link step (a common anti-pattern) mixes
phases. it is an ontological error: you are applying a concept
to an entity it does not govern. that it happens to work — because
the linker ignores most compilation flags — does not make it
correct. it will eventually break, silently or audibly.


## principle 3: every invariant written once; every variant once per variant

this is DRY applied at the correct granularity.

the invariant layer — source file collection, include paths,
base flags, the pattern rule bodies, the dependency inclusion —
is written once. there is one $(wildcard), one pattern rule body,
one -include directive.

the variant layer — EXTRA_CFLAGS per variant, OBJ_DIR per variant,
BIN per variant — is written once per variant, in one place.

duplication of the rule body (the triplication pathology) signals
a violation of this principle. the repair is always one of:
target-specific variables + define/endef (architecture 3 from
document 5), or BUILD_TYPE conditionals (architecture 2).
the choice between them depends on whether a single invocation
must build all variants simultaneously.


## principle 4: metaprogramming threshold

$(eval) and $(foreach) metaprogramming is justified when and
only when the generator is simpler than the generated text.

3 explicit pattern rule heads:

```makefile
$(REL_OBJ_DIR)/%.o:  $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
$(DBG_OBJ_DIR)/%.o:  $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
$(ASAN_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
```

vs the generator producing them:

```makefile
define VARIANT_RULE
$(OBJ_DIR_$(1))/%.o: $(SRC_DIR)/%.cpp ; $$(COMPILE_OBJ)
endef
$(foreach v,$(VARIANTS),$(eval $(call VARIANT_RULE,$(v))))
```

for 3 variants: the explicit form is simpler. a reader unfamiliar
with the pattern can read the explicit form directly. the eval
form requires understanding the generation logic before the rules
are visible — the reader must mentally execute the macro to see
what is actually declared.

for 10 variants: the generator is simpler. the explicit form is
10 nearly-identical lines; each additional variant requires 1
edit in 2 places (the rule head and the variant configuration).
the generator requires 1 edit in 1 place (the variant list).

the threshold is crossed when the explicit form becomes a
maintenance burden. before that threshold, the explicit form is
the elite choice: it is structurally transparent, requires no
knowledge of macro evaluation semantics to read, and produces
no surprises under `make -p` inspection.

metaprogramming that generates rules obscures structure. use it
when it genuinely reduces complexity. never use it to appear
sophisticated.


## principle 5: fail loudly at parse time

Make has a mechanism for parse-time precondition enforcement:
$(error), $(warning), and conditional guards. use them.

```makefile
ifeq ($(CXX),)
  $(error CXX is not set)
endif

ifndef MAKE_VERSION
  $(error GNU Make required; POSIX make detected)
endif
```

these run during phase 1, before any compilation. a missing
toolchain, an unset required variable, an incompatible Make
version — all are detectable before any disk access. a Makefile
that silently proceeds and produces a cryptic compiler error
3 files in is a worse tool than one that identifies its
precondition failure immediately.

the corollary: $(info) for debugging is fine during development.
remove all $(info) calls before committing. they execute on
every make invocation including `make clean` and `make -n`.


## principle 6: silence the infrastructure; echo the build

the @ prefix is a semantic signal, not aesthetic decoration.
it communicates: this command is infrastructure — you do not
need to see it; it does not help you understand what is
happening or reproduce a failure.

infrastructure: @mkdir -p $(@D), @rm -f, @cp, @echo.
build: $(CXX) invocations, $(LD) invocations.

a developer whose build fails must be able to copy the failing
command from the terminal output and reproduce it manually.
this requires that compilation and link commands are visible.
it does not require that directory creation is visible.

the Linux kernel convention (Q variable toggling verbosity)
is appropriate for projects with CI consumers. for development
makefiles, explicit @ on infrastructure commands is sufficient
and cleaner.

never suppress build commands by default unless you also provide
a V=1 verbose mode. a silent failure leaves no trail.


## principle 7: .PHONY is not optional

every non-file target must be declared .PHONY. the cost is
zero lines (batch the declaration at the top or bottom). the
benefit is immunity to the class of silent failures produced
by a file on disk with the same name as a phony target.

this is not a risk to be weighed. it is a 1-line declaration
with no downside. omitting it is never justified.

collect all phony targets in a single declaration:

```makefile
.PHONY: all clean fclean re debug asan
```

declaring them incrementally (each target declaring its own
.PHONY) is acceptable; a single aggregated declaration is
easier to audit.


## principle 8: := for derived variables; = only when deferred expansion is required

the default in Make is =, which defers expansion. it is the
wrong default for most variables in a well-structured Makefile.

a variable whose value is computed from other variables and
should not change once defined must use :=. deferred expansion
allows a variable to see definitions that appear later in the
file — which looks convenient but conceals the evaluation order,
making the Makefile harder to reason about.

the discipline: define configuration variables first (SRC_DIR,
OBJ_DIR, CXX, base flags), then derive all compound variables
(SRC_FILES, OBJ_FILES, DEP_FILES) with :=, in order. any
attempt to use a derived variable before its inputs are defined
then fails visibly, rather than producing an empty result silently.

use = only when deferred expansion is genuinely required:
recursively self-referential variables, or variables that
must see later-defined values. document the reason.


## principle 9: Make cannot track its own rule changes

this is not a configuration error. it is a design boundary.
Make checks whether targets are stale relative to their file
prerequisites. it does not check whether the rule that produces
a target has changed. changing CXXFLAGS does not invalidate .o
files; Make has no mechanism to know that the recipe changed.

the consequences:
- changing base flags without `make re` produces a binary
  built from a mix of old and new compilation flags. the
  binary is incorrect and Make reports it as up-to-date.
- this failure is silent. no error, no warning.

the remedies, in increasing order of correctness:

**discipline**: treat any change to CXXFLAGS, LDFLAGS, or
the Makefile itself as requiring `make re` before the next
build. this is the practical default for solo and small-team
projects.

**stamp files**: a .flags file that records the current
CXXFLAGS string. if the file content changes, all .o files
see a stale prerequisite and recompile. described in
4_make-idioms-and-folklore.md. correct but adds complexity.

**content-hash build system (Shake, Bazel)**: self-tracking
by design. a rule is stale if its inputs, its flags, or its
code changed. this is the logically correct solution.
it requires leaving Make.

for the projects in this knowledge base: discipline is the
correct remedy. the cost of `make re` is low; the complexity
of stamp files is not worth it at this scale. document the
limitation clearly in team workflow.


## principle 10: the blunt prerequisite is always wrong

```makefile
# wrong
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
```

listing all headers as a prerequisite of every .o is a
correctness approximation, not a correctness solution. it
produces correct results (in the sense that no stale artifact
is missed) but violates minimality: any header change triggers
a full recompile of every TU, regardless of which headers each
TU actually includes.

the defect deepens in multi-variant builds: a single header
touch forces a full recompile of every TU in every variant.
combined with recursive make or triplication, the cost compounds.

the blunt prerequisite exists because -MMD did not exist, or
was not known, or felt too complex. it is never correct once
-MMD is available. the correct prerequisite list for any TU
is the exact set of headers included, transitively, by that TU.
this is derivable only by the preprocessor. -MMD makes the
preprocessor derive it as a side effect of compilation.
use it. the blunt prerequisite has no legitimate use case in
a project where the compiler supports -MMD.


## checklist

a Makefile satisfies noetic purity when every item below holds. 
any violation is a concrete defect with a traceable root cause.

graph completeness:
- [ ] -MMD -MP used in every compilation rule
- [ ] -include $(DEP_FILES) present, with the dash
- [ ] no blunt prerequisites anywhere in the file

phase separation:
- [ ] CXXFLAGS contains only compilation-phase flags
- [ ] LDFLAGS contains only link-phase flags
- [ ] span flags (sanitisers, coverage) declared explicitly
      and composed into both phases independently

structural stability:
- [ ] source files collected via $(wildcard) or equivalent
- [ ] OBJ_FILES derived from SRC_FILES, not manually listed
- [ ] adding a .cpp requires zero Makefile edits

DRY:
- [ ] pattern rule body written once (define/endef if multi-variant)
- [ ] each variant's configuration in exactly one location
- [ ] no duplication of rule structure across variants

correctness under reachable states:
- [ ] every non-file target declared .PHONY
- [ ] variants have separate objdirs and binary names
- [ ] clean removes .d files alongside .o files
- [ ] directory creation uses order-only prerequisites or
      @mkdir -p $(@D) inline — not a file-timestamped target

expressiveness:
- [ ] := used for all derived variables
- [ ] = used only where deferred expansion is documented
- [ ] metaprogramming ($(eval), $(foreach)) justified by
      variant count or complexity — not by aesthetics

observability:
- [ ] infrastructure commands suppressed with @
- [ ] compilation and link commands echoed
- [ ] $(error) guards on required preconditions
- [ ] no $(info) calls in committed code

known limits:
- [ ] CXXFLAGS changes require `make re` — this is documented
      in team workflow, not silently depended upon


## what the checklist does not cover

this checklist establishes the floor, not the ceiling.

it does not address: content-hash staleness detection, hermetic
builds, distributed caching, dynamic (non-static) dependency
graphs, or cross-compilation. these are the concerns where
Make's design is structurally insufficient and the correct
response is a different tool: Bazel, Nix, Shake.

the floor is: a Makefile that is correct, minimal, structurally
stable, and transparent. the checklist delivers this. the ceiling
is a build system that enforces hermeticity, tracks rule changes,
and guarantees bit-for-bit reproducibility. Make cannot reach it.
the elite Makefile practitioner knows both where the floor is
and where the ceiling is, and does not confuse tool mastery with
architectural sufficiency.