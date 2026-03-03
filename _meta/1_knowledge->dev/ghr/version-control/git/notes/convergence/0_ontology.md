# Convergence: Ontology

## The Problem Before Version Control

Work produces artifacts. Artifacts change over time. Multiple agents modify artifacts concurrently.

3 fundamental problems:
1. **Recovery** — return to prior states
2. **Attribution** — who changed what, when, why
3. **Reconciliation** — combine concurrent modifications

Version control systems exist to solve these. Git solves all three, but its architecture is particularly shaped by (3).

---

## Why a DAG?

A **Directed Acyclic Graph** emerges from one principle: *commits reference their parents*.

Each commit contains:
- A tree (snapshot of all files)
- Pointer(s) to parent commit(s)
- Metadata (author, timestamp, message)

This parent-reference creates edges. Acyclicity is guaranteed: you cannot reference a commit that doesn't yet exist.

Why not a linear sequence? Because **causality is partial, not total**.

Commit B depends on A if B was constructed from A's state. But commits B and C, both children of A, have no causal relation to each other. Neither "comes before" the other in any meaningful sense. They are *concurrent*.

```
    A
   / \
  B   C
```

A linear history would force a false ordering. The DAG represents reality: B and C are independent developments rooted in A.

---

## Divergence

**Divergence** = existence of multiple paths from a common ancestor to different current states.

It arises naturally from:
- Multiple developers working simultaneously
- Single developer maintaining parallel lines (features, experiments)
- Distributed repositories that sync intermittently

Divergence is not a problem to avoid. It is the *default condition* of concurrent work. Branches are merely named pointers into the DAG, making certain divergent lines easy to reference.

---

## Convergence

**Convergence** = reconciling divergent histories into unified state.

The fundamental question: given states B and C (both descended from A), produce state D that incorporates the work of both.

```
      A
     / \
    B   C
     \ /
      D
```

This is a **pushout** in category-theoretic terms: D is the universal object completing the square. "Universal" means: any other object that B and C both map into must factor through D.

Practically: D contains "everything" from both B and C, relative to their common starting point A.

---

## The Core Algorithm: Three-Way Merge

All convergence strategies ultimately invoke this:

```
Inputs: states A (ancestor), B (ours), C (theirs)

1. δ_B = diff(A, B)    // changes we made
2. δ_C = diff(A, C)    // changes they made
3. D = apply(A, δ_B, δ_C)
```

**Conflict** arises when δ_B and δ_C modify the same region differently. The algorithm cannot decide; human judgment required.

**Automatic merge** succeeds when:
- δ_B and δ_C touch disjoint regions, OR
- δ_B and δ_C make identical changes to same region

---

## Terminology

| Term | Meaning |
|------|---------|
| **LCA** | Least Common Ancestor — the most recent commit reachable from both divergent tips. The "A" in three-way merge. |
| **merge base** | Git's term for LCA |
| **ours** | The branch you're on (receiving changes) |
| **theirs** | The branch being merged in |
| **tip** | The most recent commit on a branch |
| **ref** | A named pointer to a commit (branches, tags, HEAD) |
| **fast-forward** | Pointer movement when no divergence exists |
| **merge commit** | Commit with two or more parents |

---

## Telos

Why converge at all? Why not maintain indefinite parallel lines?

1. **Integration** — eventually, work must combine into deployable/usable artifact
2. **Shared truth** — team needs common reference point for coordination  
3. **Cognitive limit** — tracking many divergent lines exceeds human capacity

The *when* and *how* of convergence are strategic choices. The *that* is necessity.

---

## What Convergence Strategies Decide

Given that convergence must happen, strategies differ on:

| Dimension | Options |
|-----------|---------|
| **Topology** | Preserve true DAG structure vs. linearize |
| **Granularity** | Keep all commits vs. collapse into fewer |
| **Attribution** | Preserve original authorship vs. reassign |
| **Timing** | Converge frequently vs. batch at end |

These are genuine tradeoffs, not "best practices." Context determines which tradeoffs serve your purposes.