# Merging vs Rebasing vs Squashing

What each operation IS:

## Merge

Combines two histories by creating new commit with two parents.

Before:
main:    A---B---C
              \
feature:       D---E

After merge:
main:    A---B---C-------F (merge commit)
              \         /
feature:       D---E---

Essence: Preserves both histories. Makes confluence explicit.


## Rebase

Rewrites commits. Takes your changes, replays them on top of different base.

Before:
main:    A---B---C---G
              \
feature:       D---E

After rebase:
main:    A---B---C---G
                      \
feature:               D'---E' (new commits, different hashes)

Essence: Changes history. Your commits now pretend they started from G, not B.
Critical: D' ≠ D. Different commit hash. Different parent. Same changes.


## Squash

Combines multiple commits into one.

Before:
feature: D (add file) --- E (fix typo) --- F (add tests)

After squash:
feature: X (entire feature in one commit)
Essence: Reduces granularity. Many commits become one.


## When to use each:

Rebase: Keep feature branch current with main (mid-development).

Requirement: Only rebase commits that aren't shared/merged


Merge: Integrate two equal branches (rare in this workflow).


Squash: Clean up messy feature history before integration.

Turn 47 "WIP" commits into 3 logical commits