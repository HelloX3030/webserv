# architecture: ifeq conditionals


## mechanism

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


---


## phase semantics

the ifeq blocks execute during phase 1 (variable expansion).
by the time rules are evaluated, OBJ_DIR and BIN hold their
variant-specific values.

OBJ_FILES and DEP_FILES are derived from OBJ_DIR with :=,
so they expand immediately after OBJ_DIR is set. this is
correct provided the ifeq block precedes these derivations —
definition order matters for :=.


---


## invocation model

this is a single-invocation, single-binary architecture.
one `make` builds exactly one variant. building all variants
requires separate invocations:

```
make
make BUILD_TYPE=debug
make BUILD_TYPE=asan
```

each invocation is independent with its own incremental state.
this is correct, and for most development workflows sufficient.


---


## scope

for 42 projects and typical single-developer work, this
architecture is appropriate.

the limitation: you cannot `make all` to build every variant
in a single invocation with full parallelism. that requires
target-specific variables.
