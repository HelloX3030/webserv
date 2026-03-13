# architecture: target-specific variables


## mechanism

target-specific variables propagate to the entire dependency
subgraph rooted at a target. this enables a single `make all`
to build multiple variants with shared pattern rules.

```makefile
$(RELEASE_BIN): EXTRA_CFLAGS  :=
$(RELEASE_BIN): EXTRA_LDFLAGS :=

$(DEBUG_BIN):   EXTRA_CFLAGS  := -DDEBUG=1 -g -O0
$(DEBUG_BIN):   EXTRA_LDFLAGS :=

$(ASAN_BIN):    EXTRA_CFLAGS  := -fsanitize=address -g \
                                  -fno-omit-frame-pointer
$(ASAN_BIN):    EXTRA_LDFLAGS := -fsanitize=address
```

every rule in the subgraph rooted at a variant binary sees
that variant's EXTRA_CFLAGS and EXTRA_LDFLAGS during phase 2.


---


## the objdir obstacle

OBJ_DIR must also vary per variant — but target-specific
variables are active only during phase 2. prerequisite lists
expand during phase 1, when OBJ_DIR has only its global value:

```makefile
$(DEBUG_BIN): $(OBJ_FILES)   # wrong: expands with global OBJ_DIR
```

resolution: each variant requires an explicit object list
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


---


## factoring the rule body

3 pattern rules are needed — Make matches by the literal
directory prefix in the target path. but the rule bodies are
identical, differing only in EXTRA_CFLAGS (already variant-scoped).
use define/endef to write the body once:

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
COMPILE_OBJ expands at recipe execution time (phase 2), when
the target-specific value is active.

result: one rule body, 3 pattern heads, 3 fully separated
artifact namespaces. `make all` builds all 3 simultaneously
under -j.


---


## dependency files

each variant's .d files land in that variant's objdir alongside
the .o files. the -include directive must cover all 3:

```makefile
DEP_FILES := $(REL_OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(ASAN_OBJS:.o=.d)
-include $(DEP_FILES)
```

the dash in -include suppresses errors on first build when no
.d files exist. from the second build onward, each variant has
its own header dependency graph, independently tracked.

a header change triggers recompilation in every variant whose
.o files include it — correct, because each variant's .d files
record that variant's actual #include graph.


---


## eliminating the 3 rule heads

the 3 explicit pattern rule heads exist because Make's pattern
matching is syntactic: the directory prefix in `obj_debug/%.o`
is literal text, not an expanded variable.

$(eval) metaprogramming eliminates even these:

```makefile
VARIANTS         := release debug asan
OBJ_DIR_release  := obj
OBJ_DIR_debug    := obj_debug
OBJ_DIR_asan     := obj_asan

define VARIANT_RULE
$(OBJ_DIR_$(1))/%.o: $(SRC_DIR)/%.cpp ; $$(COMPILE_OBJ)
endef

$(foreach v,$(VARIANTS),$(eval $(call VARIANT_RULE,$(v))))
```

the $$ escapes COMPILE_OBJ expansion to phase 2 — necessary
because $(eval) runs during phase 1.

trade-off: the 3-line explicit form is immediately readable;
the foreach/eval form is DRY across arbitrarily many variants.
the cost is debuggability — `make -p` shows generated rules,
not the generator. correct choice depends on variant count
and team context.


---


## clean targets

```makefile
.PHONY: clean fclean

clean:
	rm -rf $(REL_OBJ_DIR) $(DBG_OBJ_DIR) $(ASAN_OBJ_DIR)

fclean: clean
	rm -f $(RELEASE_BIN) $(DEBUG_BIN) $(ASAN_BIN)
```

variant-specific clean targets are optional; warranted when
iteration on a single variant is the common workflow and
full fclean is too expensive.
