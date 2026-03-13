# Git Switch

## The Problem: Overloaded `checkout`

Historically, `git checkout` performed 3 semantically distinct operations:

1. Switch branches
2. Create and switch to new branch  
3. Restore files from history

This violated single-responsibility principle. 
One command, multiple operations.

## Git 2.23 (2019): The Semantic Split

Git introduced commands with clear, singular purposes:

- **`git switch`** - Branch operations only
- **`git restore`** - File operations only
- **`git checkout`** - Legacy (still valid for backward compatibility)

## Git Switch: Complete Semantics

### Basic Branch Switching

```bash
git switch main
git switch feature-branch
```

**Operation:** Move HEAD to point to specified branch reference.

```
Before: HEAD → refs/heads/current-branch → commit-A
After:  HEAD → refs/heads/main → commit-B
```

Working directory updates to match commit-B's tree.

### Create and Switch

```bash
git switch -c feature
# or verbose
git switch --create feature
```

**Flag meaning:** `-c` = `--create`

**Precise semantics:** Create a new branch reference at the current commit, 
then move HEAD to point to this new branch.

**Decomposes to:**
1. `git branch feature` (creates branch reference at current commit)
2. `git switch feature` (moves HEAD to point to it)


### Detach HEAD

```bash
git switch --detach <commit>
git switch --detach a3f5c8
```

**Flag meaning:** `--detach`

**Precise semantics:** Move HEAD to point directly at commit object, 
bypassing branch reference layer.

```
Normal state:   HEAD → refs/heads/main → commit
Detached state: HEAD → commit (direct)
```

The flag explicitly requests detachment from branch reference structure. 
HEAD no longer tracks a branch—it points directly to a commit in the DAG.

### Return to Previous Branch

```bash
git switch -
```

**Semantics:** Switch to the branch HEAD pointed to before the last switch operation.

Uses reflog (`.git/logs/HEAD`) to track previous positions. Equivalent to:
```bash
git switch @{-1}
```

The `-` is shorthand for "previous position in reflog".

## Ontological Comparison: Switch vs Checkout

| Operation | Semantic Intent | Legacy Command | Modern Command |
|-----------|----------------|----------------|----------------|
| Move HEAD to branch | Branch traversal | `git checkout main` | `git switch main` |
| Create + switch | Branch creation + traversal | `git checkout -b name` | `git switch -c name` |
| Detach HEAD | Direct commit access | `git checkout <hash>` | `git switch --detach <hash>` |
| Restore file | Working tree mutation | `git checkout -- file` | `git restore file` |
| Unstage file | Index mutation | `git checkout HEAD -- file` | `git restore --staged file` |

### The Ambiguity Problem

If `feature` exists as both a branch name and a filename:

```bash
git checkout feature
```

**Ambiguous:** Are you switching to branch `feature` or restoring file `feature`?

Git guesses based on context. Sometimes wrong.

With modern commands:

```bash
git switch feature     # Unambiguous: branch operation
git restore feature    # Unambiguous: file operation
```

Semantic clarity eliminates ambiguity.

## Other Switch Options

### Switch to Remote Branch

```bash
git switch feature-branch
```

If `feature-branch` doesn't exist locally 
but exists as `origin/feature-branch`, git automatically:
1. Creates local branch `feature-branch`
2. Sets up tracking relationship with `origin/feature-branch`
3. Switches to new local branch

Equivalent to:
```bash
git checkout -b feature-branch origin/feature-branch
```

### Force Switch (Discard Local Changes)

```bash
git switch --discard-changes main
# or
git switch --force main
```

**Semantics:** Switch branches even if working directory has uncommitted changes. 
Discards those changes.

**Use with caution.** Uncommitted work is lost.

### Switch with Merge Conflict Resolution

```bash
git switch --merge main
```

**Semantics:** If switching would overwrite uncommitted changes, 
attempt three-way merge instead of aborting.

Useful when you have work-in-progress and need to switch branches without committing or stashing.

## Why This Matters

### Clarity of Intent

```bash
# Old way - what's the intent?
git checkout feature-branch

# New way - crystal clear
git switch feature-branch      # I'm changing branches
git restore feature-branch     # I'm restoring a file
```

### Reduced Cognitive Load

Learning `switch` + `restore` teaches git's conceptual model better than learning overloaded `checkout`.

New users understand:
- Branch operations are distinct from file operations
- HEAD movement is separate from working tree modification

### Better Tooling

IDEs and scripts can reason about `switch` vs `restore` semantically.

`checkout` requires parsing arguments to infer intent—brittle and error-prone.


## Essential Understanding

Commands should have singular, clear semantics. 
Git's maturation toward `switch` and `restore` reflects evolution toward semantic precision.

The underlying operations haven't changed—git's object model remains identical. 
Only the interface has clarified what operations mean and what effects they have.

## Flag Reference

| Flag | Full Form | Precise Meaning |
|------|-----------|-----------------|
| `-c` | `--create` | Create new branch reference at current commit, then switch to it |
| `-C` | `--force-create` | Create branch (overwriting if exists), then switch to it |
| `-d` | `--detach` | Move HEAD to point directly at commit (bypassing branch reference) |
| `-f` | `--force` | Switch even if working directory has uncommitted changes (discards them) |
| `-m` | `--merge` | Attempt three-way merge if switch would overwrite local changes |
| `-` | - | Switch to branch at previous HEAD position (uses reflog) |