# Coherent Evolution of Structured Information

## The Problem

Information exists. It changes. Multiple agents change it concurrently.

How do we maintain coherence?

---

## Primitives

**State**: A configuration of information at a moment. A snapshot.

**Change**: A transformation from one state to another. Also called: delta, patch, edit, mutation, transition.

**Agent**: An entity that produces changes. Human, process, node, system.

**Coherence**: The result is internally consistent. No contradiction. Usable as unified whole.

---

## The Concurrency Problem

If changes occur sequentially, coherence is simple: each change operates on the result of the previous.

```
S₀ → S₁ → S₂ → S₃
```

Total order. No ambiguity.

But agents act concurrently. Agent A modifies state. Agent B modifies state. Neither knows of the other's action until later.

```
      S₁ (A's result)
     /
S₀ →
     \
      S₂ (B's result)
```

Now what? S₁ and S₂ both exist. Both are "current." Neither is wrong.

This is **divergence**.

---

## Time Under Concurrency

Sequential thinking assumes global time: event X happens before event Y, or after, or simultaneously.

Concurrency breaks this. If A and B act independently, their actions have no temporal relation. Neither is "before" the other in any meaningful sense.

**Causality replaces time.** Event X causally precedes Y if Y depends on X — if Y could not have occurred without X having occurred first.

Causal precedence is a **partial order**: some events are comparable (one caused the other), some are not (independent).

This is why version control uses DAGs, not linear logs.

---

## Convergence

Given divergent states, produce a unified state that incorporates all changes.

```
      S₁
     /  \
S₀ →      → S₃
     \  /
      S₂
```

S₃ must:
1. Contain A's work (relative to S₀)
2. Contain B's work (relative to S₀)
3. Be internally consistent

This is the **convergence problem**.

---

## When Convergence Fails: Conflict

A changes region R to value X.
B changes region R to value Y.
X ≠ Y.

No automatic resolution. The system cannot decide. Human judgment required.

Conflict is not failure of the system. It is the system correctly identifying that the problem is underdetermined.

---

## Mathematical Structures

### Partial Orders

A set with a relation ≤ that is:
- Reflexive: a ≤ a
- Antisymmetric: if a ≤ b and b ≤ a, then a = b
- Transitive: if a ≤ b and b ≤ c, then a ≤ c

Causality forms a partial order. The DAG of states/events is a partial order.

### Categories

Objects: States
Morphisms: Changes (state → state)
Composition: Sequential application of changes
Identity: The "no change" change

Merge as **pushout**: Given S₀ → S₁ and S₀ → S₂, the pushout S₃ is the universal state that both S₁ and S₂ map into.

"Universal" means: any other state that S₁ and S₂ both map into must factor through S₃. S₃ is the "smallest" or "most economical" merge.

### Lattices

A partially ordered set where any two elements have:
- A **join** (least upper bound): smallest element ≥ both
- A **meet** (greatest lower bound): largest element ≤ both

If states form a lattice, merge = join. Always exists, always unique, always consistent.

CRDTs exploit this: design data structures where states form a join-semilattice. Merge is then automatic and conflict-free.

### Algebraic Structure on Changes

Changes can be:
- **Composed**: Apply A then B → combined change AB
- **Inverted**: For change A, there exists A⁻¹ that undoes it
- **Commuted**: Sometimes AB = BA (order doesn't matter)

If changes form a group (closure, associativity, identity, inverses), many properties become tractable.

Darcs formalizes this. Git approximates it.

---

## Instantiations

The same problem, different domains:

| Domain | State | Change | Convergence mechanism |
|--------|-------|--------|----------------------|
| Version control | File tree | Commit/patch | Three-way merge, rebase |
| Distributed DB | Database state | Transaction | Consensus protocols, MVCC |
| Collaborative editing | Document | Edit operation | OT, CRDT |
| Distributed filesystem | Directory tree | File operation | Sync protocols |
| Blockchain | Ledger | Transaction | Proof-of-work consensus |

Each domain has developed its own solutions. The underlying problem is identical.

---

## The Fundamental Tradeoff

**CAP theorem** (Brewer): A distributed system cannot simultaneously guarantee:
- **Consistency**: All nodes see same state
- **Availability**: Every request receives a response
- **Partition tolerance**: System operates despite network splits

You get two. Pick which one to sacrifice.

Git chooses: Partition tolerant + Available. Consistency is eventual (you must explicitly pull/merge).

Centralized VCS chose: Consistent + Available. No partition tolerance (server down = no work).

---

## Coherence, More Precisely

What does "internally consistent" mean?

**Logical consistency**: No proposition P such that both P and ¬P are derivable.

**Structural consistency**: Data satisfies its invariants. A tree is still a tree. References point to existing objects.

**Semantic consistency**: The information still means what it should. Harder to formalize. Often requires human judgment.

Automatic merge can guarantee structural consistency. Semantic consistency is undecidable in general.

---

## Open Questions

1. Is there a universal algebra of changes that subsumes patches, OT operations, CRDT operations?

2. What is the minimal structure required for conflict-free convergence?

3. How does coherence in information systems relate to coherence in physics, logic, consciousness?

4. Can we formalize "semantic consistency" for restricted domains?

---

## Connections

→ **Logic**: Consistency of theories, paraconsistent logics
→ **Distributed systems**: Consensus, replication, CAP theorem
→ **Category theory**: Limits, colimits, pushouts
→ **Order theory**: Lattices, partial orders
→ **Epistemology**: Multiple observers, shared reality
→ **Physics**: Gauge coherence, reference frames, relativity of simultaneity