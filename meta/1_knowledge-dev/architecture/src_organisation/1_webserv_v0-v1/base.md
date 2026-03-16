## predicate

dependency floor. domain-agnostic primitives.
nothing here depends on domain code (HTTP, config, networking).

test: "does X depend on any domain concept?" if yes, it doesn't belong.

---

## contents

```
Fd.cpp      RAII wrapper for POSIX file descriptors
format.cpp  string formatting
log.cpp     diagnostics infrastructure
utils.cpp   path resolution
```

---

## naming

"base" encodes position in the dependency graph: the floor.

rejected: "utils"
- implies convenience ("useful in multiple places")
- weak predicate, no constraint, becomes dumping ground

---

## v0 → v1

removed: `signal.cpp` → `core/`
shutdown semantics belong with lifecycle orchestration.
