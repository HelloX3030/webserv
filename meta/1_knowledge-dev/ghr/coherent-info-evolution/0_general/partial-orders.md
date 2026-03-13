# Partial Orders and Causality

## Why Not Total Order?

A **total order** on a set S means: for any a, b ∈ S, either a ≤ b or b ≤ a.

Real numbers have this. Clock time (naively) has this.

But concurrent events have no such ordering. If A acts in Berlin and B acts in Tokyo, neither action is "before" the other — there is no fact of the matter until they interact.

This is not a limitation of our knowledge. It is the structure of reality under concurrency.

---

## Partial Orders

A **partial order** on set S is a relation ≤ satisfying:

1. **Reflexivity**: a ≤ a
2. **Antisymmetry**: a ≤ b ∧ b ≤ a → a = b  
3. **Transitivity**: a ≤ b ∧ b ≤ c → a ≤ c

Two elements a, b are **comparable** if a ≤ b or b ≤ a.
Otherwise they are **incomparable**: a ∥ b.

A partial order where all elements are comparable is a total order. Total order is special case.

---

## Happens-Before (Lamport)

Leslie Lamport, 1978: "Time, Clocks, and the Ordering of Events in a Distributed System."

Defines **happens-before** relation → on events:

1. If a and b are events in the same process, and a occurs before b, then a → b.
2. If a is the sending of a message and b is the receipt of that message, then a → b.
3. Transitivity: a → b ∧ b → c → a → c.

If neither a → b nor b → a, then a and b are **concurrent**: a ∥ b.

This is a partial order. It captures causality, not clock time.

---

## DAGs as Partial Orders

A **Directed Acyclic Graph** is a set of nodes with directed edges, no cycles.

The reachability relation (can you get from a to b following edges?) is a partial order.

Git's commit graph is a DAG. Commits are nodes. Parent pointers are edges (pointing backward in time).

If commit A is ancestor of B: A ≤ B (A "happened before" B causally).
If neither is ancestor of other: A ∥ B (concurrent development).

---

## Least Common Ancestor (LCA)

Given partial order (S, ≤) and elements a, b:

A **common ancestor** of a and b is any c such that c ≤ a and c ≤ b.

The **least common ancestor** is the greatest among all common ancestors: the most recent point where the histories were unified.

```
        c (LCA)
       / \
      /   \
     a     b
```

LCA is the natural base for three-way merge: it represents the last shared state before divergence.

In simple DAGs, LCA is unique. In complex merge histories, multiple LCAs can exist. Git handles this with recursive merge strategy.

---

## Upper Bounds and Joins

Given a, b in partial order (S, ≤):

An **upper bound** of a and b is any u such that a ≤ u and b ≤ u.

The **least upper bound** (lub) or **join** of a and b, written a ∨ b, is the smallest upper bound: an upper bound u such that for any other upper bound v, u ≤ v.

The join, if it exists, is unique.

**Merge is the computation of a join.** Given divergent states a and b, produce their least upper bound — the smallest state containing both.

---

## Lattices

A **join-semilattice** is a partial order where every pair of elements has a join.

A **meet-semilattice** is a partial order where every pair has a meet (greatest lower bound).

A **lattice** has both joins and meets for all pairs.

If your state space forms a join-semilattice, merge is always defined, always unique, always automatic.

This is the key insight behind CRDTs.

---

## Implications for Version Control

Git's state space (set of all possible file trees) is NOT a natural lattice. Two trees can diverge such that no unique minimal merge exists. Hence: conflicts.

CRDTs are designed so the state space IS a lattice. Tradeoff: restricted operations, restricted expressiveness.

General version control lives in the uncomfortable middle: expressive states, no lattice structure, conflicts possible.

---

## Connection to Physics

Special relativity: simultaneity is relative. Events spacelike-separated have no objective time ordering.

The causal structure of spacetime is a partial order. Light cones define what can influence what.

Distributed systems recapitulate this structure. Network delays create "light cones" of information propagation.

Lamport's happens-before is the software analogue of relativistic causality.