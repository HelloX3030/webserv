# 4. make idioms and folklore

Folklore in a technical community is accumulated practical
wisdom — patterns discovered through pain, anti-patterns
identified by their failure modes, idioms that encode hard-won
understanding in reusable form. Make has 50 years of it.
This document catalogues what matters: the idioms that are
logically necessary and the anti-patterns that are logical
violations, with root causes traced in both directions.


## anti-patterns

### the blunt header prerequisite

```makefile
# wrong
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

Every object file depends on every header. This is a
sound over-approximation — it never misses a rebuild —
but it is maximally pessimistic. Touch any header,
rebuild the world. At 60-120 TUs this is intolerable.

Root cause: the programmer is declaring dependencies
manually, at a coarser granularity than the compiler
sees. The fix is not to maintain a more accurate manual
list — it is to delegate to the entity with complete
information. See: the -MMD idiom below.

The critical interaction: if -MMD is added without
removing $(H_FILES) from the prerequisite list, the
blunt dependency survives alongside the precise one.
The blunt prerequisite dominates — full rebuild on any
header change, exactly as before. Both must not coexist.

### recursive make

```makefile
# wrong
subsystem:
	$(MAKE) -C src/http
	$(MAKE) -C src/config
```

Recursive make spawns a child make process per
subdirectory, each with its own isolated dependency
graph. The parent graph does not see the child graphs.
Consequence: inter-subsystem dependencies — src/config
depending on an artifact in src/http — cannot be
expressed in the dependency graph. They are encoded
implicitly in the ordering of the recursive calls. This
is the imperative/declarative conflation from document 2,
imported into the build structure itself.

Peter Miller's 1997 paper diagnosed this systematically.
The failure modes: incorrect incremental builds across
subsystem boundaries, inability to parallelise across
subsystems, and make -j producing races that are
invisible to each isolated child graph.

The fix is non-recursive make: a single top-level
Makefile with the complete dependency graph, composed
from included .mk fragments per subsystem. Covered in
depth in 5_build-variant-architectures.md.

Reference: Miller, P. "Recursive Make Considered Harmful."
https://aegis.sourceforge.net/auug97.pdf

### flags conflated across compilation and link phases

```makefile
# wrong
$(NAME): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@
```

Passing CXXFLAGS (compilation-phase flags) to the
link step is a phase category error. Most compilation
flags (-Wall, -Wextra, -std=c++17, -O2) are silently
ignored by the linker. The danger is in the exceptions:
-fsanitize flags must appear at both compile and link
time. If CXXFLAGS contains -fsanitize and is passed to
the link step, this happens to work — but only because
the linker recognises it, not because it is CXXFLAGS.
When CXXFLAGS does not contain -fsanitize (the release
build), the linker gets no sanitiser flag, and everything
is correct. When CXXFLAGS does contain it (the sanitiser
build), relying on CXXFLAGS propagation to the link step
is fragile: it works until someone refactors the flags,
at which point the sanitiser build silently breaks.

The correct model: CXXFLAGS for compilation, LDFLAGS
for linking, and a separate SAN_FLAGS (or similar)
composed explicitly into both phases where needed.
Detailed in 5_build-variant-architectures.md.

### variables defined after their := dependents

```makefile
# wrong
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
SRC_DIR   := src   # too late — OBJ_FILES already expanded with empty SRC_DIR
SRC_FILES := ...
```

:= expands immediately at the assignment line. Any
variable referenced on the right-hand side must be
defined before this line. This is the most common
source of empty variables and silent build failures
in large Makefiles. The fix: define all input variables
(SRC_DIR, OBJ_DIR, SRC_FILES) before any := expression
that references them. Establish a disciplined declaration
order: configuration variables first, then derived
variables, then rules.

### missing .PHONY declarations

A target without .PHONY is treated as a filename. If
a file with that name exists, Make considers the target
up-to-date and skips its rule — silently. `make clean`
does nothing if a file named `clean` exists. `make all`
does nothing if a file named `all` exists. This is not
hypothetical: generated build artifacts, documentation
files, or careless `touch all` commands produce exactly
this situation.

Cost of .PHONY: zero. Benefit: immunity to this class
of failure. There is no argument for omitting it from
any non-file target.

### $(shell ...) in hot paths

```makefile
# expensive if called frequently
COMMIT := $(shell git rev-parse --short HEAD)
```

$(shell ...) executes at parse time — on every make
invocation, including `make clean` and `make --dry-run`.
If the shell command is slow (git operations, find on
large trees, network calls), every make invocation pays
this cost, even when no rebuild is needed. Minimise
$(shell ...) calls; cache their results if reused;
consider whether the information is actually needed at
parse time or only in a specific target's action.

### silent failure in rule actions

```makefile
# dangerous
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@
```

If mkdir fails (permissions issue, path too long, disk
full), Make will proceed to the compilation step — it
does not check the exit code of commands that do not
fail the rule. In GNU Make, each line of a rule action
runs in a separate sub-shell; a non-zero exit code on
any line causes the rule to fail and Make to stop, but
only if the command is not prefixed with `-`. This is
correct by default — but knowing the mechanism matters
for cases where intentional failure suppression (the
`-` prefix) is needed.

```makefile
-rm -f stale_artifact  # failure here is expected and suppressed
```

Use `-` only when failure of that specific command is
genuinely acceptable. Do not use it as a blanket
suppressor of unknown failures.


## idioms

### the -MMD -MP dependency generation idiom

The complete pattern:

```makefile
CXXFLAGS  := -Wall -Wextra -Werror -std=c++17 -MMD -MP
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEP_FILES)
```

Note: no $(H_FILES) in the prerequisite list. The .d
files provide the prerequisites for headers. The rule
depends only on the source file — the first build
compiles without header dependency tracking; subsequent
builds use the .d files generated by the first. This
first-build gap is acceptable: on the first build,
all targets are stale regardless, so a missing dependency
cannot cause an incorrect incremental build.

The -include line must appear after the pattern rule
in the Makefile — Make processes includes at parse time
in textual order, but since .d files contain explicit
rules (not pattern rules), their inclusion after the
pattern rule is fine. The explicit rules in .d files
take precedence over pattern rules for specific targets.

### stamp files: a partial remedy for non-self-tracking

Make does not rebuild targets when the rules that
produce them change — only when their file prerequisites
change. This means changing CXXFLAGS does not invalidate
existing .o files.

A stamp file is a technique to make rule changes visible
as file changes:

```makefile
.flags: FORCE
	@echo "$(CXXFLAGS)" | cmp -s - $@ || echo "$(CXXFLAGS)" > $@

FORCE:

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp .flags
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
```

The .flags file contains the current CXXFLAGS string.
On each make invocation, it is regenerated only if the
flags have changed (cmp -s compares silently — no update
if identical). If CXXFLAGS changes, .flags is updated,
all .o files see a stale prerequisite, and a full
recompile follows.

This is not elegant — it is a workaround for a genuine
limitation of the mtime-based, non-self-tracking model.
It works, but adds complexity. The alternative is
discipline: treat any CXXFLAGS change as requiring
`make re`. For a production system requiring correctness
guarantees, use a self-tracking build system (Shake,
Bazel) instead.

### target-specific variables

Make allows variables to be scoped to a specific target
and its dependencies:

```makefile
$(DEBUG_NAME): EXTRA_FLAGS := -DDEBUG=1 -g
$(DEBUG_NAME): $(DBG_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) $(DBG_OBJ_FILES) -o $@

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) $(INCLUDES) -c $< -o $@
```

EXTRA_FLAGS is set to `-DDEBUG=1 -g` for the debug target
and all rules in its dependency subgraph. For other targets,
EXTRA_FLAGS is empty (or whatever the global default is).
This enables a single pattern rule to serve multiple build
variants without duplication — the mechanism for eliminating
the triplicated rules in Lukas' current Makefile. Full
treatment in 5_build-variant-architectures.md.

### silent rules with @ and .SILENT

Prefixing a rule action with @ suppresses its echo:

```makefile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
```

The mkdir is silent; the compilation command is echoed.
The principle: suppress noisy infrastructure commands
(directory creation, cleanup, checks) that do not convey
build progress; echo meaningful build commands so the
developer sees what the compiler is doing and can
reproduce any failing command manually.

An alternative convention: fully silent by default,
with a verbose mode via a flag variable:

```makefile
V ?= 0
ifeq ($(V),0)
  Q := @
else
  Q :=
endif

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(Q)mkdir -p $(@D)
	$(Q)$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
```

`make V=1` enables full verbosity. This is the convention
used by the Linux kernel Makefile and by many large
open-source projects. It is the more professional pattern
for systems that will be built in CI/CD environments,
where verbose output is noise, and by developers who
want clean output.

### the $(error ...) and $(warning ...) idioms

$(warning ...) and $(error ...) emit messages during
phase 1 (parse time). $(error ...) is fatal — Make
aborts immediately. $(warning ...) is informational.

```makefile
ifeq ($(CXX),)
  $(error CXX is not set. Set it to your C++ compiler.)
endif

ifneq ($(filter 4.% 5.%, $(MAKE_VERSION)),)
  # GNU Make 4+, safe to use $(file ...)
else
  $(warning GNU Make < 4.0 detected. Some features unavailable.)
endif
```

These run at parse time — before any compilation begins.
They are the correct mechanism for validating preconditions
(required tools present, required variables set, required
Make version available) and failing fast with a clear
message rather than producing a cryptic rule failure.

$(info ...) is the non-fatal variant, useful during
Makefile debugging to print variable values:

```makefile
$(info SRC_FILES = $(SRC_FILES))
$(info OBJ_FILES = $(OBJ_FILES))
```

Remove $(info ...) calls before committing — they execute
on every make invocation, including in CI.

### the define / endef multi-line variable idiom

```makefile
define COMPILE_RULE
@mkdir -p $(@D)
$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) $(INCLUDES) -c $< -o $@
endef

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_RULE)

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_RULE)
```

define / endef creates a multi-line variable. Expanding
it in a rule action expands each line as a separate
command. Combined with target-specific variables, this
eliminates duplication of rule action sequences across
build variants while keeping each instantiation
independently configurable via EXTRA_FLAGS.

This is the idiomatic DRY pattern for rule actions in
Make — the closest Make gets to first-class functions
for the imperative layer.

### the non-recursive include pattern

Instead of recursive make, compose the build from
included fragments:

```makefile
# top-level Makefile
include src/http/module.mk
include src/config/module.mk
include src/runtime/module.mk
```

Each module.mk contributes to the shared OBJ_FILES
list and declares its own source files, with paths
relative to the project root. The top-level Makefile
accumulates all object files and links them. One make
process, one dependency graph, correct incremental
builds across module boundaries, full -j parallelism.

The trade-off: module.mk files must use project-root-
relative paths, which is slightly awkward but not
difficult. Miller's paper argues the discipline is
worth it at any non-trivial scale.

### VPATH and vpath: source search paths

```makefile
VPATH = src:include
# or, more precisely:
vpath %.cpp src
vpath %.hpp include
```

VPATH tells Make where to search for prerequisites
that are not found relative to the current directory.
This enables out-of-tree builds — source files in one
directory, build artifacts in another, without rewriting
every source path explicitly.

vpath (lowercase) is the more precise form: it scopes
the search path to a specific pattern. Multiple vpath
directives can establish different search paths for
different file types.

For projects where sources and objects are already
separated by explicit path construction ($(SRC_DIR)/
and $(OBJ_DIR)/), VPATH is not needed. It becomes
relevant when building against external source trees
or when implementing fully out-of-tree builds where
the source directory is read-only.


## portability: POSIX make vs GNU make

A Makefile that uses GNU Make extensions will not work
with BSD make (macOS default make prior to Homebrew),
nmake (Windows), or strictly POSIX environments.
For projects targeting only Linux development machines
(as webserv does), GNU Make is universally available
and its extensions are safe to use. For portable
libraries or tools, know the boundary.

GNU Make only (not POSIX):
- $(foreach ...), $(call ...), $(eval ...)
- target-specific variables
- $(shell ...) (POSIX has no equivalent)
- pattern rules with % (POSIX has suffix rules instead)
- .SECONDEXPANSION
- $(file ...)
- Order-only prerequisites (|)
- $(info ...), $(warning ...), $(error ...)

POSIX portable:
- := and = assignment (= is POSIX; := is POSIX 2012+)
- Suffix rules (.c.o:) — the POSIX predecessor to
  pattern rules
- $@, $<, $* automatic variables (POSIX specifies these)
- include directive (POSIX 2012+)
- .PHONY (POSIX 2012+)

In practice: write for GNU Make, note extensions used,
test with `make --warn-undefined-variables` to surface
latent issues.


## debugging a Makefile

`make -n` (dry run): print the commands that would be
executed without executing them. Essential for verifying
that the correct rules fire for a given target and that
variable expansions produce the expected commands.

`make -p` (print database): print the complete rule
and variable database after parsing — all variables,
all rules including implicit rules, all targets. The
output is enormous but unambiguous. Use it to diagnose
why a rule is or is not firing, or to see what implicit
rules Make is considering.

`make --debug=all`: print Make's internal decision-making
— why each target is considered stale or up-to-date,
which rules are being evaluated for which targets. More
readable than -p for staleness debugging.

`make --warn-undefined-variables`: warn whenever an
undefined variable is expanded. Invaluable for diagnosing
silent empty-variable bugs — a missing $(SRC_DIR) that
silently produces wrong paths rather than an error.

$(info ...) at strategic points during parse: already
noted above. Print SRC_FILES and OBJ_FILES immediately
after their definition to verify the wildcard/patsubst
logic produces the expected lists.


## language comparisons

Haskell — shake's typed idioms:
Shake replaces Make's string-based variable system
with Haskell's type system. A Shake rule is a Haskell
function with a typed result. The equivalent of Make's
target-specific variable — scoping a flag to a build
variant — is a Haskell closure: the function capturing
its environment at definition time, with no possibility
of the wrong environment being in scope. Make's biggest
source of folklore bugs (wrong variable at use time,
lazy evaluation surprise, blunt prerequisites) simply
do not exist in Shake because the type system prevents
them. The stamp file idiom has no equivalent in Shake:
Shake is self-tracking by construction.

Rust — build.rs and Cargo's flag model:
Cargo separates compilation flags (passed to rustc)
from link flags (passed to the linker) explicitly in
its build model. There is no CXXFLAGS/LDFLAGS conflation
possible because the API is typed — `cargo:rustc-flags`
and `cargo:rustc-link-lib` are distinct declarations.
This is the typed-interface solution to the phase
conflation anti-pattern. Cargo is also fully self-tracking:
changing any Cargo.toml field or build.rs output
triggers a rebuild of all affected crates.

Guile Scheme — the eval danger:
Make's $(eval) is analogous to Scheme's eval — with none
of Scheme's hygiene. In Scheme, macro systems (syntax-rules,
syntax-case) provide hygienic expansion: a macro cannot
accidentally capture variable names from its use site.
Make's $(eval) expands into the global Makefile namespace
with no scoping. Variables defined inside a $(eval)
become global. This is the correct analogy for why
$(eval)-based metaprogramming in Make is fragile: it
is unhygienic macro expansion in a dynamically scoped
language. Guile's macro system is a model of what Make's
metaprogramming could be, if Make had a real language
underneath it — which is precisely Guix's answer
(replace Make's metaprogramming with Guile itself).

Agda — totality and the completeness gap:
In Agda, a total function must handle every case in
its domain — the type checker rejects partial functions.
Make's dependency graph has no totality enforcement:
a missing dependency declaration is not an error at
Makefile-parse time. It is only detected (sometimes,
non-deterministically) at build time, under specific
conditions. The analogy: a Make dependency graph is a
partial function from "requested targets" to "correct
artifacts" — partial because missing edges mean some
inputs do not produce correct outputs. A build system
with totality enforcement (e.g. Bazel's sandbox, which
makes undeclared inputs inaccessible) would reject an
incomplete dependency graph the way Agda rejects a
partial function. This is the deepest connection between
the type-theoretic and build-system correctness concerns
relevant to this knowledge base.


## sources

Miller, P. "Recursive Make Considered Harmful." 1997.
https://aegis.sourceforge.net/auug97.pdf
The foundational anti-pattern paper. Every pathology
of recursive make that folklore has documented since
traces back to this analysis.

GNU Make manual, chapters 5-8.
https://www.gnu.org/software/make/manual/
Rule syntax, variable assignment, conditionals, and
functions. The reference for all idioms described here.

"Implementing non-recursive make" — various community
sources. The Miller paper describes the principle;
practical implementations are discussed extensively
in the autotools and cmake communities. Search:
"non-recursive make example" for concrete implementations.

Mitchell, N. "Shake Before Building." ICFP 2012.
https://dl.acm.org/doi/10.1145/2364527.2364538
The typed alternative to Make's folklore-heavy idioms.
Shake's design is a catalogue of Make's limitations
addressed constructively.

Linux kernel Makefile.
https://github.com/torvalds/linux/blob/master/Makefile
The most sophisticated Make system in production.
The verbose/silent Q variable convention, the modular
include-based architecture, and the multi-variant build
system are all visible here. Reading it with the
knowledge in documents 0-4 is an advanced exercise.