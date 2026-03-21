# std::map

## essence

an associative container mapping keys to values,
maintaining keys in sorted order at all times.

```cpp
std::map<Key, Value>
```

each key appears at most once. lookup, insertion, and
deletion are O(log n) in the number of entries.

the invariant is total ordering: for any 2 keys a, b,
exactly 1 of a < b, a == b, b < a holds.
the container enforces this continuously.


---


## underlying structure

the standard mandates O(log n) for lookup, insert, erase.
the universal implementation is a self-balancing binary
search tree — in every major standard library (libstdc++,
libc++, MSVC), a red-black tree.

a red-black tree maintains balance through 2 invariants:
1. every path from root to leaf has the same number of
   black nodes (black-height uniformity).
2. no 2 consecutive red nodes on any path.

these together bound tree height to 2 log₂(n + 1),
guaranteeing O(log n) worst-case on all operations.

the tree structure makes sorted iteration a by-product:
in-order traversal yields keys in ascending order at no extra cost.


---


## ordering

`std::map<K, V>` requires a strict weak ordering on K.
by default: `std::less<K>`, which calls `operator<`.

strict weak ordering requires:
- irreflexivity: ¬(a < a)
- asymmetry: a < b → ¬(b < a)
- transitivity: a < b ∧ b < c → a < c
- transitivity of incomparability:
  ¬(a < b) ∧ ¬(b < a) ∧ ¬(b < c) ∧ ¬(c < b) → ¬(a < c) ∧ ¬(c < a)

keys are equivalent iff neither is less than the other.
the map uses this equivalence — not `operator==` — to
determine identity. this matters for custom comparators.

a custom comparator can be passed as a 3rd template
parameter, enabling maps over types without `operator<`,
or maps with non-default ordering (e.g. case-insensitive).


---


## key interface
```cpp
// lookup — does not insert
auto it = m.find(key);       // iterator or m.end()
bool present = m.count(key); // 0 or 1

// insertion
m.insert({key, value});      // no-op if key exists
m.emplace(key, value);       // construct in-place, no-op if exists
m[key] = value;              // inserts default if key absent, then assigns

// erase
m.erase(key);                // by key; no-op if absent
m.erase(it);                 // by iterator; UB if it == end()

// iteration
for (auto& [k, v] : m) { }  // in ascending key order
```

`operator[]` silently inserts a default-constructed value
if the key is absent. on a `const` map it does not compile.
`find` is the correct lookup when insertion on miss is not desired.


---


## in webserv

`HttpRequest::headers` is `std::map<std::string, std::string>`,
keys normalised to lowercase at parse time.

lookup is by exact string key:
```cpp
auto it = headers.find("content-length");
```

ordering is incidental — the map is not iterated in sorted
order for any purpose. an unordered alternative
(`std::unordered_map`) would give O(1) average lookup
at the cost of no ordering guarantee. for the number of
headers in a typical HTTP request (order of 10s), the
difference is negligible. `std::map` was chosen for
simplicity: no hash function required for `std::string`.


---


## language perspectives

### Agda

no mutable map in the standard library; purely functional maps are finger trees
or AVL trees via `Data.Map` in Haskell's sense. in Agda, association lists
suffice for small structures; dependently typed maps can carry proofs
of key membership.

### Haskell

`Data.Map.Strict` — a purely functional AVL tree.
immutability means every insert returns a new map;
the old map is unchanged. structural sharing makes this efficient:
only the path from root to the modified node is copied.

```haskell
import qualified Data.Map.Strict as Map

m  = Map.fromList [("host", "example.com")]
m' = Map.insert "content-length" "42" m
-- m unchanged, m' is a new map sharing structure with m
```

### Rust

`std::collections::BTreeMap` — sorted, O(log n), B-tree
(not red-black). `HashMap` — unordered, O(1) average.

```rust
use std::collections::BTreeMap;
let mut m: BTreeMap<String, String> = BTreeMap::new();
m.insert("content-length".to_string(), "42".to_string());
```

Rust's ownership model prevents iterator invalidation by construction:
holding an iterator borrows the map, preventing concurrent mutation.


---


## references

cppreference: https://en.cppreference.com/w/cpp/container/map

Meyers, S. Effective STL (2001). item 19: understand the
difference between equality and equivalence.

ISO/IEC 14882 (C++ standard), section 26.5 (associative containers).
