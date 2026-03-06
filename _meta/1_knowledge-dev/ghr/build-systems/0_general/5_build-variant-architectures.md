# build variant architectures


## what a build variant is, ontologically

a build variant is a realisation of the same source tree under a
different configuration of the transformation pipeline. source files
are identical across variants; what varies is the set of flags
passed to the compilation and link phases, the output directory,
and the binary name.

exactly 4 things vary between any 2 variants:

- compilation flags: additions to the base CXXFLAGS for this
  variant only. examples: -DDEBUG=1, -g, -O0, -fsanitize=address.
  these affect .o production.

- link flags: additions to the base LDFLAGS for this variant only.
  examples: -fsanitize=address (sanitiser runtime injection),
  -lgcov (coverage instrumentation). critical: sanitiser flags
  must appear here AND in compilation flags — discussed below.

- object directory: the directory into which .o and .d files land.
  variants must have separate objdirs. if release and debug share
  an objdir, a debug recompilation overwrites release .o files
  while leaving them timestamped as newer than their sources —
  the release binary becomes silently inconsistent.

- binary name: the output executable name. variants must have
  separate names to coexist on disk.

everything else — source files, include paths, base CXXFLAGS and
LDFLAGS, the dependency graph structure, the pattern rules — is
invariant. polluting the invariant layer with variant-specific
concerns is the root pathology of ad-hoc multi-variant Makefiles.
every architecture below is a strategy for representing the
invariant parts once and the variant parts once per variant.


## the sanitiser correctness constraint

sanitiser flags (-fsanitize=address, -fsanitize=undefined, etc.)
must appear at both compilation and link time. they are not merely
compilation flags. the linker must inject the sanitiser runtime
library, which requires -fsanitize to be present in the link
invocation.

consequence: a single CXXFLAGS variable cannot be the sole carrier
of sanitiser flags if it is used only for compilation. if CXXFLAGS
is also passed to the link step (the phase-conflation error
documented in 4_make-idioms-and-folklore.md), the sanitiser build
happens to work — but for the wrong structural reason, and breaks
when the conflation is corrected.

the correct model: CXXFLAGS governs compilation, LDFLAGS governs
linking, and a dedicated variable (EXTRA_CFLAGS, EXTRA_LDFLAGS)
carries variant-specific additions into each phase explicitly.
sanitiser flags are set in both.


## architecture 1: separate Makefile per variant

one file per variant; all shared logic is duplicated in full.

this is not an architecture — it is an absence of one. its only
virtue is that each file is independently readable. every change
to shared logic (source list, include paths, base flags) must be
replicated across all files. an omitted replication is a silent
divergence. mentioned for completeness; not used.


## architecture 2: BUILD_TYPE variable with ifeq conditionals

a single Makefile selects variant configuration at parse time
via a user-supplied variable:

```makefile
BUILD_TYPE ?= release

ifeq ($(BUILD_TYPE),debug)
  EXTRA_CFLAGS  := -DDEBUG=1 -g -O0
  EXTRA_LDFLAGS :=
  BIN           := webserv_debug
  OBJ_DIR       := obj_debug
else ifeq ($(BUILD_TYPE),asan)
  EXTRA_CFLAGS  := -fsanitize=address -g -fno-omit-frame-pointer
  EXTRA_LDFLAGS := -fsanitize=address
  BIN           := webserv_asan
  OBJ_DIR       := obj_asan
else
  EXTRA_CFLAGS  :=
  EXTRA_LDFLAGS :=
  BIN           := webserv
  OBJ_DIR       := obj
endif

OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

$(BIN): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

-include $(DEP_FILES)
```

usage: `make BUILD_TYPE=debug`, `make BUILD_TYPE=asan`, `make`.

the ifeq blocks execute during phase 1 (variable expansion).
by the time rules are evaluated, OBJ_DIR and BIN have their
variant-specific values. OBJ_FILES and DEP_FILES are derived
from OBJ_DIR with :=, so they expand immediately after OBJ_DIR
is set — correct, provided the ifeq block precedes these
derivations (definition order matters for := as established
in 3_make-semantics.md).

this is a single-invocation, single-binary-per-invocation
architecture. one `make` builds exactly one variant. building
all variants requires separate invocations: `make`, then
`make BUILD_TYPE=debug`, then `make BUILD_TYPE=asan`. each
invocation is independent with its own incremental state; this
is correct and for most development workflows sufficient.

for 42 projects and typical single-developer work, architecture
2 is the appropriate scope.


## architecture 3: target-specific variables with per-variant objdirs

target-specific variables (specified in 3_make-semantics.md,
introduced practically in 4_make-idioms-and-folklore.md)
propagate to the entire dependency subgraph rooted at a target.
this enables a single `make all` to build multiple variants with
shared pattern rules.

the declaration:

```makefile
$(RELEASE_BIN): EXTRA_CFLAGS  :=
$(RELEASE_BIN): EXTRA_LDFLAGS :=

$(DEBUG_BIN):   EXTRA_CFLAGS  := -DDEBUG=1 -g -O0
$(DEBUG_BIN):   EXTRA_LDFLAGS :=

$(ASAN_BIN):    EXTRA_CFLAGS  := -fsanitize=address -g \
                                  -fno-omit-frame-pointer
$(ASAN_BIN):    EXTRA_LDFLAGS := -fsanitize=address
```

each variant binary target sets its own EXTRA_CFLAGS and
EXTRA_LDFLAGS. every rule in the subgraph rooted at that
target sees those values during phase 2 execution.

### the objdir obstacle

OBJ_DIR must also vary per variant — but target-specific variables
are active only during phase 2. the prerequisite lists on each
variant binary target are expanded during phase 1, when OBJ_DIR
has only its global value. the expression:

```makefile
$(DEBUG_BIN): $(OBJ_FILES)   # wrong: OBJ_FILES uses global OBJ_DIR
```

expands OBJ_FILES at parse time with the wrong OBJ_DIR.

the resolution: each variant requires an explicit object list
computed with its specific directory:

```makefile
REL_OBJ_DIR  := obj
DBG_OBJ_DIR  := obj_debug
ASAN_OBJ_DIR := obj_asan

REL_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(REL_OBJ_DIR)/%.o,$(SRC_FILES))
DBG_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,$(DBG_OBJ_DIR)/%.o,$(SRC_FILES))
ASAN_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(ASAN_OBJ_DIR)/%.o,$(SRC_FILES))
```

each variant binary then depends on its own list:

```makefile
$(RELEASE_BIN): $(REL_OBJS)
	$(CXX) $(CXXFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(DEBUG_BIN): $(DBG_OBJS)
	$(CXX) $(CXXFLAGS) $(EXTRA_LDFLAGS) $^ -o $@

$(ASAN_BIN): $(ASAN_OBJS)
	$(CXX) $(CXXFLAGS) $(EXTRA_LDFLAGS) $^ -o $@
```

### factoring the shared rule body

3 separate pattern rules, one per objdir, are needed — Make
matches pattern rules by the literal directory prefix in the
target path. but the rule bodies are identical: they differ only
in EXTRA_CFLAGS, which is already variant-scoped via
target-specific propagation. use define/endef to write the body
once:

```makefile
define COMPILE_OBJ
@mkdir -p $(@D)
$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@
endef

$(REL_OBJ_DIR)/%.o:  $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
$(DBG_OBJ_DIR)/%.o:  $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
$(ASAN_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
```

define/endef captures text, not expansions. EXTRA_CFLAGS inside
COMPILE_OBJ is expanded at recipe execution time (phase 2), when
the target-specific value is active. this is the key: define/endef
defers the expansion of the body to the moment of use, which is
exactly when the target-specific context is live.

result: one rule body, 3 pattern heads, 3 fully separated artifact
namespaces. a single `make all` can build all 3 simultaneously
under `-j`.

### dependency files across multiple objdirs

each variant's .d files land in that variant's objdir alongside
the .o files. the -include directive must cover all 3:

```makefile
DEP_FILES := $(REL_OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(ASAN_OBJS:.o=.d)
-include $(DEP_FILES)
```

the dash in -include suppresses errors on first build when no
.d files exist. from the second build onward, each variant has
its own correct header dependency graph, independently tracked.
a header change triggers recompilation in every variant whose
compiled .o files include it — correct, because the .d files
per variant record that variant's actual #include graph.

### the 3-rule-head limitation and its resolution

architecture 3 still requires 3 explicit pattern rule heads:

```makefile
$(REL_OBJ_DIR)/%.o:  $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
$(DBG_OBJ_DIR)/%.o:  $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
$(ASAN_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp ; $(COMPILE_OBJ)
```

this is because Make's pattern matching is syntactic: the
directory prefix in `obj_debug/%.o` is literal text in the
rule head, not an expanded variable. a variable cannot appear
in a pattern rule head and be pattern-matched — the % matches
within the literal string as written.

$(eval) metaprogramming can eliminate even these 3 declarations.
a foreach loop generates the rules dynamically at parse time:

```makefile
VARIANTS     := release debug asan
OBJ_DIR_release  := obj
OBJ_DIR_debug    := obj_debug
OBJ_DIR_asan     := obj_asan

define VARIANT_RULE
$(OBJ_DIR_$(1))/%.o: $(SRC_DIR)/%.cpp ; $$(COMPILE_OBJ)
endef

$(foreach v,$(VARIANTS),$(eval $(call VARIANT_RULE,$(v))))
```

the $$ in the rule body escapes the COMPILE_OBJ expansion to
phase 2 — necessary because $(eval) runs during phase 1 and
would otherwise expand COMPILE_OBJ prematurely.

the trade-off: the 3-line explicit form is immediately readable;
the foreach/eval form is DRY across arbitrarily many variants.
the cost of the eval approach is debuggability — `make -p` output
shows the generated rules, not the generator; a reader unfamiliar
with the pattern must reverse-engineer the generation logic.
the correct choice depends on variant count and team context.
this trade-off is articulated as a principle in
6_elite-makefile-principles.md.


### clean targets

```makefile
.PHONY: clean fclean

clean:
	rm -rf $(REL_OBJ_DIR) $(DBG_OBJ_DIR) $(ASAN_OBJ_DIR)

fclean: clean
	rm -f $(RELEASE_BIN) $(DEBUG_BIN) $(ASAN_BIN)
```

variant-specific clean targets are optional; they are warranted
when iteration on a single variant is the common workflow and
full fclean is too expensive.


## architecture 4: configuration via included .mk fragments

at scale, variant configuration can be extracted into separate
.mk files and composed via include:

```
config/
    release.mk
    debug.mk
    asan.mk
```

```makefile
# config/asan.mk
ASAN_CFLAGS  := -fsanitize=address -g -fno-omit-frame-pointer
ASAN_LDFLAGS := -fsanitize=address
ASAN_BIN     := webserv_asan
ASAN_OBJ_DIR := obj_asan
```

```makefile
# top-level Makefile
include config/release.mk
include config/debug.mk
include config/asan.mk
```

each variant's configuration is then a first-class artefact with
a location, editability, and version-control history. this is the
same include-based composition from the Miller non-recursive
pattern (document 4), applied to configuration rather than
module source lists.

for 3 variants the separation is overengineering. for 10+ variants
(sanitisers, fuzzer builds, coverage, cross-compilation targets,
CI configurations), it is the correct decomposition. the webserv
project warrants architecture 2 or 3.


## the miller non-recursive pattern in multi-variant builds

Miller's non-recursive architecture (one top-level Makefile
including module.mk fragments) and multi-variant architecture are
orthogonal concerns that compose cleanly. the module.mk files
contribute to a shared source list; the variant machinery then
computes per-variant object lists and pattern rules from it.

the composition:

```makefile
# top-level Makefile
SRC_FILES :=

include src/http/module.mk     # appends to SRC_FILES
include src/config/module.mk
include src/runtime/module.mk

# variant object lists derived from accumulated SRC_FILES
REL_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,obj/%.o,$(SRC_FILES))
DBG_OBJS  := $(patsubst $(SRC_DIR)/%.cpp,obj_debug/%.o,$(SRC_FILES))
```

one dependency graph, all modules, all variants, full -j
parallelism across and within variants.


## language comparisons

### Haskell — Shake

in Shake, a variant is a Haskell value — a record or ADT.
a rule parameterised by a variant is a function closed over
that value. the type system enforces the separation that Make
enforces only by programmer discipline: variant-specific flags
cannot reach the wrong build context without a type error.

```haskell
data Variant = Release | Debug | Asan deriving (Show, Eq)

compileFlags :: Variant -> [String]
compileFlags Release = ["-O2"]
compileFlags Debug   = ["-DDEBUG=1", "-g", "-O0"]
compileFlags Asan    = ["-fsanitize=address", "-g"]

linkFlags :: Variant -> [String]
linkFlags Asan = ["-fsanitize=address"]
linkFlags _    = []
```

no target-specific variable machinery, no define/endef workaround:
the variant is threaded explicitly as a value through every
function. the composition is transparent and type-checked.

### Rust — Cargo

Cargo's profile system (dev, release, bench, test) is precisely
this architecture, normalised into the build tool. the "variant"
concern is removed from the user's build description: you declare
which profile you want, Cargo manages separate output directories,
flag sets, and artifact names. the pathology of triplicating rules
does not arise because the tool owns the variant concern.

this is the upstream resolution: recognise that variant management
is a recurring, domain-general problem, and solve it once in the
build tool rather than in every project's Makefile. Make's not
solving it is a consequence of its design philosophy — it is a
general-purpose rule evaluator, not a build-lifecycle manager.


## sources

Miller, P. "Recursive Make Considered Harmful." 1997.
https://aegis.sourceforge.net/auug97.pdf
the single-graph argument; the include-based composition
used in the module pattern.

GNU Make manual, section 6.11: Target-specific Variable Values.
https://www.gnu.org/software/make/manual/html_node/Target_002dspecific.html
the authoritative specification: semantics, inheritance rules,
override precedence, and interaction with pattern rules.

Mitchell, N. "Shake Before Building." ICFP 2012.
https://dl.acm.org/doi/10.1145/2364527.2364538
the typed alternative; demonstrates how first-class values
eliminate the target-specific variable machinery entirely.

Cargo reference: profiles.
https://doc.rust-lang.org/cargo/reference/profiles.html
the normalised, tool-level solution to the variant problem.
read as evidence for what Make is missing by design.