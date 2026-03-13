# analysis: Lukas' Makefile


## the central question

a Makefile is correct when its dependency graph accurately
represents the true information flow of the build. every
defect in a Makefile is either a consequence of an
inaccurate graph, or a structural decision that prevents
the graph from being expressed accurately. the analysis
below works from that definition, moving from the
governing architectural choice down through its cascading
consequences to the fine details.


## the governing decision: triplication

the Makefile's structure is 3 complete, independent build
definitions — one for each variant (release, debug, leaks)
— sharing only the variable declarations at the top. each
variant has its own object list, its own binary target, its
own pattern rule. the 3 pattern rule bodies are structurally
identical except for which flags variable they reference.

this is not a design choice between alternatives. it is the
absence of a design: the variant concern has been handled by
repetition rather than parameterisation.

the invariant layer — what all 3 variants share — is: the
source files, the compiler, the include paths, the base flags,
the structure of the compilation pipeline, the rule logic. none
of this changes between variants. the variant layer — what
actually differs — is: a small set of additional flags, a
different output directory, a different binary name. the ratio
of invariant to variant is roughly 10:1 in terms of information
content. the Makefile inverts this ratio in terms of text: the
variant material is the dominant structural feature; the
invariant material appears 3 times over.

the consequence is not merely aesthetic. duplication of rule
structure means any change to the shared logic must be applied
in 3 places. an omitted application is a silent divergence
between variants — the release build compiles correctly; the
debug build silently uses an old include path. this class of
error is undetectable at build time.

the correct architecture — target-specific variables with a
shared pattern rule body via define/endef — writes the invariant once
and the variant configuration once per variant. the triplication
is the foundational structural error from which the other
defects follow.


## the dependency graph: the blunt prerequisite

given the triplication, examine what each pattern rule
actually declares:

```makefile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES)
```

H_FILES is every header in the include/ tree, collected
recursively. every .o file in the release build depends on
every header. the same holds for the debug and leaks builds.

this is the blunt prerequisite. its intent is to ensure that
.o files are recompiled when headers change — a real
requirement. its mechanism is to declare a dependency that is
never true: no single .o file depends on all headers. the
declared graph over-approximates the true graph.

the consequence of over-approximation is minimality failure:
any change to any header — including one that is included by
only 1 source file — marks every .o in every variant as stale
and triggers a full recompile of the entire project, 3 times
over (once per variant, if all 3 are built). the correctness
invariant is satisfied; the minimality invariant is violated.
the practical cost scales with project size.

the reason the blunt prerequisite was chosen is visible from
what is absent: there is no -MMD flag anywhere in the Makefile.
-MMD instructs the compiler to emit, as a side effect of
compilation, a .d file recording the exact headers transitively
included by that .cpp. those .d files, pulled in via
`-include $(DEP_FILES)`, give Make the exact, per-TU dependency
graph — the only mechanism by which minimality and correctness
can be simultaneously achieved.

without -MMD, the developer faces a genuine dilemma: either
over-approximate (blunt prerequisite — correct but not minimal)
or under-declare (no header prerequisites — minimal but
incorrect, since header changes go undetected). the blunt
prerequisite is the safer of the 2 wrong answers. but it is
still wrong. the correct answer is not a choice between these;
it is -MMD, which dissolves the dilemma by making the
information available.

the dependency graph defect therefore has 2 layers: the
absence of -MMD, which is the root, and the blunt prerequisite,
which is the symptom — a compensatory workaround that accepts
minimality failure to avoid correctness failure.


## phase conflation in the link rules

all 3 link rules pass CXXFLAGS to the compiler driver at
link time:

```makefile
$(NAME): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@

$(DEBUG_NAME): $(DBG_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(DBG_OBJ_FILES) -o $@

$(LEAK_NAME): $(LEAK_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(LEAK_FLAGS) $(LEAK_OBJ_FILES) -o $@
```

CXXFLAGS contains: -Wall -Wextra -Werror -std=c++17. these are
compilation-phase flags. the linker silently ignores them. the
build produces correct output, but for the wrong structural
reason: it works because the flags happen to be harmless at
link time, not because the phase model is correct.

the leak build additionally passes LEAK_FLAGS at link time:
-DDEBUG=1, -g, -O0, -fno-omit-frame-pointer. -DDEBUG=1 is a
preprocessor flag, irrelevant at link time. -g, -O0, and
-fno-omit-frame-pointer are code-generation flags, also
irrelevant at link time.

none of this produces a wrong binary in the current webserv
build because no span flags (flags that must appear at both
compile and link time) are in use. the leaks build targets
valgrind analysis, not address sanitiser, so -fsanitize is
absent. were asan added later — a natural evolution for a
project that already has a leak-check variant — and CXXFLAGS
used as the carrier for -fsanitize=address, the existing
structure would produce a correct asan compile but a broken
asan link, since the sanitiser runtime would not be injected.
the failure would be silent until runtime.

the correct model is: CXXFLAGS for compilation, LDFLAGS for
linking, explicit span flags composed into both phases where
needed. the link rule:

```makefile
$(NAME): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) $^ -o $@
```

LDFLAGS is currently absent from the Makefile entirely. its
absence is the phase-model defect stated at the variable level.


## the debug variant is not a debug build

DEBUG_FLAGS is defined as:

```makefile
DEBUG_FLAGS := -DDEBUG=1
```

-DDEBUG=1 defines the preprocessor macro that gates debug
log calls throughout the codebase. this is correct and
intentional: the debug binary produces diagnostic output
that the release binary suppresses.

what is missing: -g. the debug binary has no debug symbols.
it cannot be inspected with gdb in any meaningful way. symbol
names are absent from stack traces. breakpoints cannot be set
by source location. the variant is named debug and enables
debug logging, but it does not enable debuggability in the
technical sense — the information the compiler could embed
in the binary to connect machine state to source state is
not requested.

compare with LEAK_FLAGS:

```makefile
LEAK_FLAGS := -DDEBUG=1 -g -O0 -fno-omit-frame-pointer
```

the leaks variant includes -g and -O0. it is, in terms of
debug symbol production, the more complete debug build. the
naming is inverted relative to what the flags actually
produce: the build called `debug` has no debug symbols; the
build called `leaks` has full debug symbols and no
optimisation.

this is a conceptual error in the variant taxonomy. the 3
variants as designed are: release (optimised, no debug info),
debug-logging (logging enabled, otherwise like release), and
valgrind-friendly (logging enabled, full symbols, no
optimisation). a developer reaching for `make debug` expecting
a gdb-capable binary will not get one.


## the rwildcard function

source collection uses a custom recursive wildcard function:

```makefile
rwildcard = $(foreach d,$(wildcard $1*), \
              $(call rwildcard,$d/,$2) \
              $(filter $(subst *,%,$2),$d))

SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.cpp)
H_FILES   := $(call rwildcard,$(INC_DIR)/,*.hpp) \
             $(call rwildcard,$(INC_DIR)/,*.h)
```

this function is correct in intent: it collects files
recursively across subdirectories, where the built-in
$(wildcard ...) matches only a single directory level.

2 observations. first, it executes at parse time on every
make invocation — including `make clean`, `make -n`, and
`make --dry-run` — walking the entire directory tree via
repeated $(wildcard) calls. for webserv's current scale
this cost is immeasurable. for deeper trees or slower
filesystems it becomes relevant.

second, and more importantly: H_FILES is computed at parse
time and used as the blunt prerequisite. the rwildcard
function is doing real work — correctly enumerating all
headers — in service of a flawed dependency model. with
-MMD, H_FILES is not needed at all. the compiler does the
enumeration, per TU, as a side effect of the compilation
it was already performing. the parse-time tree walk is
rendered unnecessary by the correct dependency mechanism.


## clean does not remove dependency files

the clean rule:

```makefile
clean:
	$(RM) -r $(OBJ_DIR) $(DBG_OBJ_DIR) $(LEAK_OBJ_DIR)
```

removes the 3 object directories. since the Makefile generates
no .d files — -MMD is absent — there are no dependency files
to clean. the rule is complete with respect to the current
build's artifacts.

in the redesign, -MMD will be added. .d files will be
co-located with .o files inside the object directories. since
clean removes the directories recursively, the .d files will
be cleaned automatically. no change to the clean rule is
required by the -MMD addition — this is a consequence of the
design choice to co-locate .d and .o files, which ensures
clean semantics are preserved without modification.


## the re target and variant scope

```makefile
re: fclean all
```

fclean removes all 3 variants' artifacts. all builds only the
release binary. a developer running `make re` after a structural
change (a new source file, a modified header) gets a rebuilt
release binary and a cleaned-but-unbuilt debug and leaks state.
the other variants are not rebuilt; they do not exist on disk.

this is not an error — re is a 42 convention for the release
build — but it is worth naming: re does not mean "rebuild
everything"; it means "clean everything, rebuild release". in
a multi-variant build with shared source, the distinction matters.


## summary of defects by layer

**architectural:** triplication of the variant build instead
of a single parameterised build. root of the structural
fragility.

**dependency graph:** no -MMD; blunt prerequisite as
compensatory over-approximation. correctness preserved;
minimality violated. scales poorly with project size and
variant count.

**phase model:** CXXFLAGS (and LEAK_FLAGS) at link time.
currently harmless; structurally incorrect; a latent hazard
for any future asan or coverage variant. LDFLAGS absent.

**variant taxonomy:** the debug variant lacks -g; the leaks
variant is the de facto debug build. naming does not reflect
capability.

**derived:** rwildcard performs parse-time work that -MMD
would render unnecessary. clean is correct for the current
artifact set and will remain correct after -MMD is added.
re is correctly scoped to the release build by convention
but is worth documenting as such.