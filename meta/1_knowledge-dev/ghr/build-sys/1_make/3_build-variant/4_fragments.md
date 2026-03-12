# architecture: configuration fragments


## mechanism

variant configuration extracted into separate .mk files:

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

each variant's configuration becomes a first-class artefact
with a location, editability, and version-control history.


---


## when warranted

for 3 variants the separation is overengineering.

for 10+ variants — sanitisers, fuzzer builds, coverage,
cross-compilation targets, CI configurations — it is the
correct decomposition.

webserv warrants architecture 2 (ifeq) or 3 (target-specific),
not this.


---


## composition with the Miller pattern

Miller's non-recursive architecture (one top-level Makefile
including module.mk fragments) and multi-variant architecture
are orthogonal concerns that compose cleanly.

module.mk files contribute to a shared source list; variant
machinery computes per-variant object lists from it:

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


---


## sources

Miller, P. "Recursive Make Considered Harmful." 1997.
https://aegis.sourceforge.net/auug97.pdf
the single-graph argument; the include-based composition.

GNU Make manual, section 6.11: Target-specific Variable Values.
https://www.gnu.org/software/make/manual/html_node/Target_002dspecific.html
semantics, inheritance rules, override precedence,
interaction with pattern rules.
