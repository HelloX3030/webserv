# redesign: webserv Makefile


## constraints

2 sources of constraint govern the redesign: the 42 spec
and the project's source tree layout.

from the 42 spec:
- required targets: NAME (= `webserv`), all, clean, fclean, re
- compiler: `c++`
- flags: -Wall -Wextra -Werror
- standard: -std=c++17 (permitted at 42 Heilbronn)
- no unnecessary relinking

"no unnecessary relinking" is not a stylistic preference.
it is the minimality invariant restated for the link phase:
the binary must not be relinked unless a .o file, a library,
or the link command itself has changed. correct dependency
tracking is what satisfies it — not a manually curated
prerequisite list.

from the project layout:
- source files live under src/ in subdirectories
  (src/base/, src/classes/, src/interfaces/).
  a flat $(wildcard src/*.cpp) would produce an empty list.
  recursive collection is required.
- headers live under include/ in subdirectories.
  INCLUDES := -I include is sufficient — the compiler
  resolves subdirectory paths from that root; no recursive
  include flag is needed.


## variant taxonomy

before any Makefile syntax, the 3 variants must be defined
precisely. a variant is characterised by its purpose, its
compilation additions, and its link additions.

**release** — the 42-required binary.
  purpose: production-candidate build. base flags only.
  EXTRA_CFLAGS  := (none)
  EXTRA_LDFLAGS := (none)
  binary: webserv
  objdir: obj/

**debug** — developer iteration with a debugger.
  purpose: gdb-capable binary with logging enabled.
  EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
  EXTRA_LDFLAGS := (none)
  binary: webserv_debug
  objdir: obj_debug/

  correction from the prior build: the old debug variant
  had -DDEBUG=1 only, no -g. it produced logging but no
  debug symbols — not debuggable with gdb. the corrected
  variant adds -g -O0 -fno-omit-frame-pointer. -O0 disables
  optimisation so source and machine state correspond. -g
  embeds DWARF symbols. -fno-omit-frame-pointer preserves
  the frame pointer register, which gdb uses to walk the
  call stack reliably.

**leaks** — valgrind analysis.
  purpose: valgrind-compatible binary. full debug info,
  logging enabled, no optimisation.
  EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
  EXTRA_LDFLAGS := (none)
  binary: webserv_leaks
  objdir: obj_leaks/

  note: the debug and leaks variants are flag-identical.
  they are kept separate because their operational purpose
  differs — `make debug` signals "I am running gdb";
  `make leaks` signals "I am running valgrind". if an asan
  variant is added in future, leaks would diverge to carry
  -fsanitize=address in both EXTRA_CFLAGS and EXTRA_LDFLAGS,
  at which point the separation becomes structurally necessary.
  establishing the pattern now costs nothing.

  no span flags (flags required at both compile and link
  time) are in use. EXTRA_LDFLAGS is therefore empty for
  all 3 variants. LDFLAGS, the base link-phase variable,
  is also empty — webserv links no external libraries.
  both are declared explicitly: an explicit empty variable
  documents the slot. it is not an omission.


## the invariant and variant layers, explicitly

the invariant layer — what is identical across all 3 variants:
- source files (SRC_FILES)
- compiler (CXX)
- include path (INCLUDES)
- base compilation flags (CXXFLAGS, including -MMD -MP)
- base link flags (LDFLAGS, empty)
- the rule structure: compilation pipeline, .d inclusion
- clean and fclean semantics

the variant layer — what differs:
- EXTRA_CFLAGS: flags added to compilation for this variant
- EXTRA_LDFLAGS: flags added to linking for this variant
- output directory (OBJ_DIR per variant)
- binary name (NAME per variant)

the ratio is roughly 8:1. the invariant layer is written
once. the variant layer is written once per variant in
a single block. nothing else is permitted to vary.


## decision 1: source collection

the project uses subdirectories under src/. the built-in
$(wildcard ...) matches a single directory level. recursive
collection is required.

rwildcard is retained from the original Makefile. it is
correct, idiomatic GNU Make, and its parse-time cost
is immeasurable at webserv's scale. it is used once,
for SRC_FILES.

H_FILES is eliminated. it served the blunt prerequisite.
with -MMD in place, H_FILES has no role: the compiler
derives the exact per-TU header dependency set as a
side effect of compilation. the parse-time tree walk over
include/ is dropped entirely.

```makefile
rwildcard = $(foreach d,$(wildcard $1*),\
              $(call rwildcard,$d/,$2)\
              $(filter $(subst *,%,$2),$d))

SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)
```


## decision 2: dependency graph — -MMD -MP

-MMD and -MP are added to CXXFLAGS. the placement in
CXXFLAGS rather than the recipe body is a deliberate choice:
these flags must be present for every compilation, without
exception, across all 3 variants. placing them in CXXFLAGS
makes them structurally impossible to omit — any compilation
invocation that uses $(CXXFLAGS) carries them. they are
visible in the echoed compilation command, making their
presence auditable.

-MMD: instructs the compiler to emit a .d file alongside
the .o, recording the exact set of headers transitively
included by this translation unit. the .d file is a Make
rule fragment: an explicit dependency rule for this
specific .o file against its specific headers.

-MP: adds a phony target for each header listed in the .d
file. without -MP, deleting a header causes Make to error
on the next invocation — it finds the header listed as a
dependency in the .d file, the file does not exist, and
no rule to build it. -MP prevents this by making each
listed header a phony target with no prerequisites and
no recipe — Make treats it as always up-to-date and
does not error on its absence.

the .d files are co-located with their corresponding .o
files: obj/foo.d lives beside obj/foo.o, obj_debug/foo.d
beside obj_debug/foo.o, and so on. this is not aesthetic.
it means the clean rule — which removes the objdirs
recursively — removes the .d files automatically. no
separate clean step for .d files is needed.

DEP_FILES is derived from all 3 object lists:

```makefile
DEP_FILES := $(REL_OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(LKS_OBJS:.o=.d)
```

the -include directive loads all .d files from all variants:

```makefile
-include $(DEP_FILES)
```

the leading `-` suppresses errors on the first build,
when no .d files yet exist. on the first build, all
targets are stale regardless — a missing .d file cannot
cause a missed dependency on the first build, because
there is nothing incrementally correct to miss. from
the second build onward, .d files are present and loaded,
and each variant's incremental graph is exact.

the -include directive is placed after all pattern rules.
Make processes the file in textual order during phase 1.
when .d files are loaded, they contain explicit rules
for specific targets (obj/http/Request.o: ...). explicit
rules take precedence over pattern rules for the same
target. the placement after pattern rules is both
convention and the order that makes this precedence
legible: the general rule is stated first, the specific
overrides follow.


## decision 3: variable declaration order and := discipline

all configuration variables are declared before all
derived variables. all derived variables use :=. this
is logically required: := expands immediately at the
assignment line. a derived variable that references
an input not yet defined expands to an empty string,
silently. the discipline prevents this class of bug.

order:
1. toolchain: CXX, CXXFLAGS, LDFLAGS
2. paths: SRC_DIR, INC_DIR, INCLUDES
3. variant names and objdirs: NAME, DBG_NAME, LKS_NAME,
   OBJ_DIR, DBG_OBJ_DIR, LKS_OBJ_DIR
4. rwildcard function (uses =, a recursive function by
   design — this is the only legitimate use of = in
   this Makefile)
5. SRC_FILES (derived from SRC_DIR via rwildcard)
6. per-variant object lists: REL_OBJS, DBG_OBJS, LKS_OBJS
   (each derived from SRC_FILES and its variant's OBJ_DIR)
7. DEP_FILES (derived from the 3 object lists)

at step 6, all inputs — SRC_FILES, OBJ_DIR, DBG_OBJ_DIR,
LKS_OBJ_DIR — are already defined. the := expansions
are correct and deterministic.


## decision 4: phase model

CXXFLAGS carries exactly the compilation-phase flags:
code quality (-Wall -Wextra -Werror), standard (-std=c++17),
and the dependency instrumentation (-MMD -MP).

LDFLAGS carries the link-phase flags. for webserv it is
empty: no external libraries, no linker customisation.
it is declared explicitly as an empty variable.

EXTRA_CFLAGS and EXTRA_LDFLAGS carry variant-specific
additions into each phase. they are set per-variant via
target-specific variables (decision 5) and composed into
each phase explicitly in the rule bodies.

the link rule uses:
```makefile
$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@
```

CXXFLAGS does not appear in the link rule. this is the
phase separation stated concretely: compilation flags
are not passed to the linker. the fact that the linker
silently ignores most of them is irrelevant — correctness
is not established by the absence of an error. it is
established by the rule being structurally correct.
were -fsanitize=address added to CXXFLAGS in a future
variant, the link rule above would correctly pass it
only if it also appears in EXTRA_LDFLAGS — which would
be explicit by design, not accidental.


## decision 5: variant architecture

architecture 3 is applied: target-specific variables
with a shared compilation rule body via define/endef,
with separate objdirs and object lists per variant.

the 3 object lists must be computed at parse time (phase 1)
because prerequisite lists are expanded during phase 1.
target-specific variables are active only during phase 2.
a variable cannot appear in a pattern rule head and be
matched by %. these are the constraints established by
Make's 2-phase evaluation model.

consequence: each variant requires an explicit object
list, computed at parse time with its specific directory.
the 3 object lists are:

```makefile
REL_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DBG_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SRC_FILES))
LKS_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(LKS_OBJ_DIR)/%.o,$(SRC_FILES))
```

each variant binary target depends on its own list:

```makefile
$(NAME):     $(REL_OBJS)
$(DBG_NAME): $(DBG_OBJS)
$(LKS_NAME): $(LKS_OBJS)
```

the target-specific variable assignments — one block
per variant, immediately before the link rule — scope
EXTRA_CFLAGS and EXTRA_LDFLAGS to each variant's entire
dependency subgraph:

```makefile
$(NAME):     EXTRA_CFLAGS  :=
$(NAME):     EXTRA_LDFLAGS :=

$(DBG_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(DBG_NAME): EXTRA_LDFLAGS :=

$(LKS_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(LKS_NAME): EXTRA_LDFLAGS :=
```

when Make evaluates the debug variant's subgraph during
phase 2, every rule in that subgraph — both the link rule
and every compilation rule for the debug .o files — sees
EXTRA_CFLAGS := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer.
the release subgraph sees EXTRA_CFLAGS := (empty). the
propagation is automatic, driven by Make's target-specific
scoping semantics.


## decision 6: the shared compilation rule body

3 pattern rule heads are required — one per objdir —
because Make's pattern matching is syntactic. the directory
prefix in obj_debug/%.o is literal text; it cannot be
a variable. this is an intrinsic constraint of pattern
rules in GNU Make.

the rule bodies are structurally identical across all 3
variants: only EXTRA_CFLAGS differs, and that is already
handled by target-specific variables. define/endef
captures the body once:

```makefile
define COMPILE_OBJ
@mkdir -p $(@D)
$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -c $< -o $@
endef
```

define/endef stores text, not an expansion. EXTRA_CFLAGS
inside COMPILE_OBJ is a textual reference. it is not
expanded when COMPILE_OBJ is defined. it is expanded when
COMPILE_OBJ is used — in a rule recipe, during phase 2 —
at which point the target-specific value of EXTRA_CFLAGS
is active. this is the timing guarantee that makes the
entire pattern work: define/endef defers expansion to the
point of use, which is exactly the point of target-specific
variable activation.

the 3 pattern rules share the body:

```makefile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(LKS_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)
```

note: @mkdir -p $(@D) creates the subdirectory structure
under each objdir mirroring the source tree. $(@D) expands
to the directory component of $@ — for obj/classes/Foo.o,
it expands to obj/classes. without this, the compilation
fails the first time any source in a subdirectory is
compiled.


## decision 7: .PHONY

all non-file targets are declared in a single .PHONY
directive at the top of the target section. a single
directive is easier to audit than scattered per-target
declarations. the complete list:

```makefile
.PHONY: all clean fclean re debug leaks \
        debugclean debugre leaksclean leaksre \
        run debugrun leaksrun
```


## decision 8: clean semantics

clean removes the 3 objdirs recursively. since .d files
are co-located with .o files inside the objdirs, they
are removed by the same operation. no separate step
for .d files is needed — this is a consequence of the
co-location design, not an accident.

fclean calls clean, then removes the 3 binaries.

re calls fclean then all. fclean removes all 3 variants'
artifacts. all builds only the release binary. `make re`
therefore rebuilds release from clean state; debug and
leaks are left absent on disk. this is the 42 convention
and it is correct: re is a release-scoped operation by
the spec's definition of `all`.

variant-specific re targets (debugre, leaksre) are provided
for the common workflow of iterating on a single non-release
variant without paying the cost of a full 3-variant clean.


## decision 9: precondition guard

`rwildcard` returns an empty string if `SRC_DIR` does not exist,
is empty, or contains no .cpp files. without a guard, the build
proceeds: `REL_OBJS`, `DBG_OBJS`, `LKS_OBJS` are all empty; the
link rule receives an empty `$^`; the linker produces a hollow
binary (or errors, depending on the linker). Make reports success.

the guard:

```makefile
ifeq ($(SRC_FILES),)
  $(error no source files found under $(SRC_DIR)/)
endif
```

this runs at parse time (phase 1), immediately after `SRC_FILES`
is derived. if `SRC_FILES` is empty, Make aborts before evaluating
any rule, with a message identifying the exact variable and path.
the failure is loud, immediate, and unambiguous.

it is placed after `SRC_FILES :=` and before the object list
derivations — the earliest point at which the emptiness is
detectable and the latest point at which catching it still
prevents all downstream silent failures.


## decision 10: verbosity model

the output model follows the linux kernel convention: silent by
default, fully verbose on `V=1`.

```makefile
V ?= 0
ifeq ($(V),0)
  Q := @
else
  Q :=
endif
```

3 categories of output, each with a distinct treatment:

**infrastructure** (`mkdir`, `rm`, `valgrind` preamble): always `@`.
these commands do not convey build progress; their output is not
reproducible-failure information. they are suppressed unconditionally,
regardless of V.

**build commands** (compiler, linker): gated by `$(Q)`. in V=0,
`Q := @` suppresses them. in V=1, `Q :=` echoes them in full —
the complete compiler invocation, every flag, every path. this is
the mode for diagnosing a compilation failure: copy the echoed
command, run it manually, inspect the output.

**informative echo lines**: always visible, never gated by Q.
format: 2-space indent, fixed-width label, then the relevant name.

```
  CXX  src/classes/HttpRequestFrontend.cpp
  LD   webserv_debug
  RM   obj obj_debug obj_leaks
```

in V=0, these lines are the complete output — a clean record of
what the build did. in V=1, they appear above the full command,
providing the same readable summary while the raw commands are
also visible.

the echo line for compilation shows `$<` (the source file) rather
than `$@` (the object file). the source file is what the developer
has mental context for; the object file path is derivable and adds
no information.

the label width (4 chars, right-padded to 5 with trailing space)
is chosen to align naturally with 2-char labels like `LD` — the
column after the label is consistently at position 7. this is
aesthetic, but deliberate: output that is read repeatedly should
be scannable.


## known limits

**non-self-tracking.** Make does not detect changes to
CXXFLAGS or EXTRA_CFLAGS as staleness signals for existing
.o files. changing any flag variable — adding -DFEATURE,
changing -O0 to -O2, modifying INCLUDES — does not
trigger a recompile. the current .o files were produced
under the old flags; Make sees their mtimes as newer
than their source files and considers them up-to-date.

the correct response to any flag change is `make re`
(or the variant-specific re target) before the next
build. this is a discipline requirement, not a mechanism.
it must be internalised and applied consistently.

**mtime proxy.** the staleness model is mtime-based.
clock skew, same-second resolution on older filesystems,
and touch-without-change are all potential sources of
false-stale or missed-rebuild. for webserv's development
context — single developer, single machine, modern ext4
filesystem — these failure modes are reachable only in
unusual circumstances and tolerable in practice.


## the Makefile

```makefile
# ─── toolchain ────────────────────────────────────────────────

CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17 -MMD -MP
LDFLAGS  :=

# ─── verbosity ────────────────────────────────────────────────
# V=0 (default): silent build with informative one-line progress.
# V=1: full command echo — every flag visible, for build debugging.
# usage: make V=1        make V=1 debug

V ?= 0
ifeq ($(V),0)
  Q := @
else
  Q :=
endif

# ─── paths ────────────────────────────────────────────────────

SRC_DIR  := src
INC_DIR  := include
INCLUDES := -I $(INC_DIR)

# ─── variant names and objdirs ────────────────────────────────

NAME     := webserv
DBG_NAME := webserv_debug
LKS_NAME := webserv_leaks

OBJ_DIR     := obj
DBG_OBJ_DIR := obj_debug
LKS_OBJ_DIR := obj_leaks

# ─── source collection ────────────────────────────────────────
# rwildcard: recursive wildcard traversal.
# used for SRC_FILES only — H_FILES is eliminated;
# header dependencies are derived per-TU by -MMD.

rwildcard = $(foreach d,$(wildcard $1*),\
              $(call rwildcard,$d/,$2)\
              $(filter $(subst *,%,$2),$d))

SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)

# ─── precondition guard ───────────────────────────────────────
# fail at parse time if no sources found — prevents a silent
# hollow-binary build from an empty or mislocated src tree.

ifeq ($(SRC_FILES),)
  $(error no source files found under $(SRC_DIR)/)
endif

# ─── derived object and dependency lists ──────────────────────
# := ensures immediate expansion after all inputs are defined.
# each variant has its own object list; DEP_FILES spans all 3.

REL_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DBG_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SRC_FILES))
LKS_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(LKS_OBJ_DIR)/%.o,$(SRC_FILES))
DEP_FILES := $(REL_OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(LKS_OBJS:.o=.d)

# ─── phony targets ────────────────────────────────────────────

.PHONY: all clean fclean re debug leaks \
        debugclean debugre leaksclean leaksre \
        run debugrun leaksrun

# ─── variant configuration ────────────────────────────────────
# target-specific variables propagate to the entire subgraph
# rooted at each binary target. EXTRA_CFLAGS is referenced in
# COMPILE_OBJ, which expands during phase 2 when these values
# are active. EXTRA_LDFLAGS is composed into each link rule.

$(NAME):     EXTRA_CFLAGS  :=
$(NAME):     EXTRA_LDFLAGS :=

$(DBG_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(DBG_NAME): EXTRA_LDFLAGS :=

$(LKS_NAME): EXTRA_CFLAGS  := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
$(LKS_NAME): EXTRA_LDFLAGS :=

# ─── 42-required targets ──────────────────────────────────────

all: $(NAME)

clean:
	@echo "  RM   $(OBJ_DIR) $(DBG_OBJ_DIR) $(LKS_OBJ_DIR)"
	@$(RM) -r $(OBJ_DIR) $(DBG_OBJ_DIR) $(LKS_OBJ_DIR)

fclean: clean
	@echo "  RM   $(NAME) $(DBG_NAME) $(LKS_NAME)"
	@$(RM) -f $(NAME) $(DBG_NAME) $(LKS_NAME)

re: fclean all

# ─── link rules ───────────────────────────────────────────────
# CXXFLAGS absent: compilation-phase flags do not belong here.
# LDFLAGS and EXTRA_LDFLAGS carry the link-phase flags.
# $^ expands to the full object list for this variant.
# echo line always visible; full command gated by Q.

$(NAME): $(REL_OBJS)
	@echo "  LD   $@"
	$(Q)$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(DBG_NAME): $(DBG_OBJS)
	@echo "  LD   $@"
	$(Q)$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(LKS_NAME): $(LKS_OBJS)
	@echo "  LD   $@"
	$(Q)$(CXX) $(LDFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

# ─── compilation rule body ────────────────────────────────────
# define/endef stores text; expansion deferred to phase 2.
# EXTRA_CFLAGS expands with the target-specific value active.
# mkdir: always @, pure infrastructure.
# echo: always visible — the readable progress signal in V=0.
# compiler invocation: gated by Q; fully visible in V=1.

define COMPILE_OBJ
@mkdir -p $(@D)
@echo "  CXX  $<"
$(Q)$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -c $< -o $@
endef

# ─── pattern rules ────────────────────────────────────────────
# 3 heads required: Make's pattern matching is syntactic;
# the directory prefix cannot be a variable. shared body
# via COMPILE_OBJ; EXTRA_CFLAGS provides variant-specific
# flags at expansion time.

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(DBG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

$(LKS_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILE_OBJ)

# ─── dependency inclusion ─────────────────────────────────────
# placed after all pattern rules. .d files contain explicit
# rules; explicit rules take precedence over pattern rules for
# the same target. the dash suppresses errors on the first
# build when no .d files exist yet.

-include $(DEP_FILES)

# ─── variant targets ──────────────────────────────────────────

debug: $(DBG_NAME)
leaks: $(LKS_NAME)

# ─── variant maintenance ──────────────────────────────────────
# variant-specific re targets for iterating on a single variant
# without paying the cost of cleaning all 3.

debugclean:
	@echo "  RM   $(DBG_OBJ_DIR) $(DBG_NAME)"
	@$(RM) -r $(DBG_OBJ_DIR)
	@$(RM) -f $(DBG_NAME)

debugre: debugclean debug

leaksclean:
	@echo "  RM   $(LKS_OBJ_DIR) $(LKS_NAME)"
	@$(RM) -r $(LKS_OBJ_DIR)
	@$(RM) -f $(LKS_NAME)

leaksre: leaksclean leaks

# ─── run targets ──────────────────────────────────────────────

run: $(NAME)
	./$(NAME)

debugrun: $(DBG_NAME)
	./$(DBG_NAME)

leaksrun: $(LKS_NAME)
	@valgrind --leak-check=full --track-fds=yes --show-leak-kinds=all \
	  --error-exitcode=1 ./$(LKS_NAME)
```
