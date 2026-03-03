# The Tension: Topology vs. Linearity

## Two Conceptions of "History"

**History as forensic record:**
> The DAG captures *what actually happened*. Branch points, merge points, concurrent work — all real events. Erasing or rearranging them falsifies the record.

**History as narrative:**
> The DAG is implementation detail. What matters is the *logical sequence of changes* that produced the current state. A clean story aids understanding; raw event logs obscure it.

Neither is wrong. They optimize for different purposes.

---

## What Topology Preservation Gives You

```
A ← B ← C ← D ← M   (merge commit M)
     \       /
      E ← F
```

- **Forensic accuracy**: "These two lines of work existed concurrently and were joined at M"
- **Bisect fidelity**: `git bisect` can identify bugs introduced in either branch
- **Authorship clarity**: Each commit retains its original SHA, timestamps, author
- **Reversibility**: Can "undo" a merge by reverting M; branch structure still visible

---

## What Linearization Gives You

```
A ← B ← C ← D ← E' ← F'   (rebased: E and F replayed as E' and F')
```

- **Readability**: `git log` shows single path; easy to follow
- **Simpler bisect**: Linear sequence → binary search without branch navigation
- **"Clean" history**: No merge commits cluttering the log
- **Atomic integration**: Feature appears as coherent unit

---

## The Core Tradeoff

| Preserve Topology | Linearize |
|-------------------|-----------|
| True record | Useful narrative |
| Complex but accurate | Simple but constructed |
| Safe for shared branches | Requires rewriting |
| More merge commits | Fewer/no merge commits |

---

## Lukas's Question, Formalized

> "It seems to destroy the concept of branches."

Linearization doesn't destroy branches — it *erases evidence* of them in the DAG.

The branch *existed* during development. Work proceeded in parallel. But upon convergence, rebase rewrites commits as if they were serial all along.

Is this "lying"? Depends on your epistemology of version control:
- If the DAG is meant to be *ground truth* → yes, it's falsification
- If the DAG is meant to be *useful abstraction* → no, it's editing for clarity

---

## When Each Makes Sense

**Preserve topology (true merge):**
- Long-running branches with meaningful independent existence
- Multiple contributors on the branch (rewriting affects everyone)
- Release branches, maintenance branches
- When you *need* to undo the integration later
- Projects valuing forensic archaeology

**Linearize (rebase/squash):**
- Short-lived feature branches
- Solo work before sharing
- "Polish before publish" workflow
- Projects valuing clean `git log`
- When individual WIP commits have no archival value

---

## The Hybrid Reality

Most teams do both:
1. **Rebase locally** to clean up personal WIP
2. **Merge to main** preserving the feature as coherent unit
3. **Never rebase shared branches** (cardinal rule)

This combines benefits: clean feature histories, true integration record.

---

## A Deeper Question

Why do we care about history at all?

1. **Debugging**: Find when/where bugs introduced
2. **Understanding**: Why does this code exist? What problem did it solve?
3. **Reverting**: Undo changes surgically
4. **Auditing**: Who approved what, when
5. **Learning**: How did the system evolve?

Different purposes weight the tradeoffs differently. A regulated industry (audit trails) weights forensic accuracy. A small team shipping fast weights readability.

There is no universal answer. There is only: *what are you optimizing for?*