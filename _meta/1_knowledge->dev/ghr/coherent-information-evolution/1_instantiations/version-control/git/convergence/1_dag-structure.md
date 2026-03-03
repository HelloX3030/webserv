# The DAG: Structure and Mechanics

## Git's Object Model

Four object types, content-addressed by SHA-1 hash:

| Object | Contains | Identity |
|--------|----------|----------|
| **blob** | File contents (raw bytes) | SHA-1 of content |
| **tree** | List of (mode, name, SHA) tuples pointing to blobs/trees | SHA-1 of that list |
| **commit** | Tree pointer + parent pointer(s) + metadata | SHA-1 of all that |
| **tag** | Pointer to object + metadata | SHA-1 of that |

A **commit** is therefore:
```
tree <sha>
parent <sha>        # zero, one, or multiple
author <name> <email> <timestamp>
committer <name> <email> <timestamp>

<message>
```

The tree pointer gives you the complete filesystem state at that commit. The parent pointer(s) embed the commit in the DAG.

---

## Refs: Naming Points in the DAG

The DAG is addressed by SHA hashes — inhuman, but precise.

**Refs** provide human-readable names:
- `.git/refs/heads/main` → file containing one SHA
- `.git/refs/remotes/origin/main` → tracks remote state
- `.git/HEAD` → usually contains `ref: refs/heads/main` (symbolic ref)

A branch is *just a file* containing 40 hex characters. "Moving a branch" = overwriting that file.

```bash
cat .git/refs/heads/main
# e.g., 7a3f8c2b9d...
```

---

## DAG Properties

**Immutability**: Once created, an object never changes. Its SHA is determined by content; changing content = different SHA = different object.

**Append-only**: You add commits; you never modify existing ones. "Rewriting history" means creating *new* commits and moving refs to point at them. Old commits remain (until garbage collected).

**Reachability**: A commit is reachable if you can traverse parent pointers from some ref to reach it. Unreachable commits are "orphaned" — still exist, but invisible to normal operations.

**Partial order**: Parent relationship defines ordering. If A is ancestor of B, then A < B. But concurrent commits B and C (both children of A) are incomparable — neither is "before" the other.

---

## Visualizing the DAG

```
A ← B ← C ← D       (linear: total order)
            ↑
          main

A ← B ← C ← D       (divergence: partial order)
     \
      E ← F
          ↑
       feature

A ← B ← C ← D ← G   (after merge: pushout)
     \       /
      E ← F
```

Arrows point backward (child → parent) because that's how Git stores it: each commit knows its parents, not its children.

---

## Finding the LCA (Least Common Ancestor)

Given two commits, find their most recent common ancestor.

Simple case:
```
    A ← B ← C
         \
          D ← E

LCA(C, E) = B
```

Complex case (criss-cross merge):
```
    A ← B ← C ← F
         \   /
          \ /
           X
          / \
         /   \
    D ← E ← G

LCA(F, G) = ?
```

Here multiple valid LCAs may exist. Git's recursive merge strategy handles this by merging the LCAs themselves first, creating a "virtual" merge base.

---

## The Reflog: Recovery Mechanism

Every ref movement is logged in `.git/logs/`.

```bash
git reflog
# shows history of where HEAD pointed
```

Even after `reset --hard`, orphaned commits remain in object store. Reflog lets you recover them (default retention: 90 days).

This is why "rewriting history" in Git is recoverable locally but dangerous when pushed — others don't have your reflog.

---

## Implications for Convergence

1. **Merge commits** have multiple parents — they are the join points in the DAG
2. **Rebase** creates *new* commits with *different* SHAs (different parent = different content = different hash)
3. **Fast-forward** requires one tip to be ancestor of other — no actual divergence to reconcile
4. **All strategies** preserve the object store; they differ only in what refs point to and what new commits are created