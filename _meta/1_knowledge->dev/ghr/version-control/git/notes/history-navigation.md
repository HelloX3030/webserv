# Git History Navigation: Full Power Over the Commit Graph

## Basic Navigation

View commit history:
```bash
git log --oneline --graph --all
```

Move to specific commit:
```bash
git checkout <commit-hash>
```

Move relative to current position:
```bash
git checkout HEAD~1    # 1 commit back
git checkout HEAD~3    # 3 commits back
git checkout HEAD^     # parent commit (^ same as ~1)
```

Return to branch tip:
```bash
git checkout main
# or
git switch main
```

## What Occurs: Detached HEAD State

**Normal state:**
```
HEAD → refs/heads/main → commit object
```

**Detached HEAD state (after checking out old commit):**
```
HEAD → commit object (directly)
```

HEAD no longer points to a branch reference—it points directly to a commit object.

**Critical insight:** The commit graph is immutable. You're not changing history;
you're moving your working directory's view through the graph. The tree structure remains unchanged.

## Full Power Commands

### Navigate by Relationship

```bash
git checkout main^        # parent of main
git checkout main^^       # grandparent
git checkout main~5       # 5 generations back
git checkout main^2       # second parent (for merge commits)
```

### Navigate by Hash

```bash
git checkout a3f5c89
```

### Navigate by Reflog

```bash
git checkout HEAD@{2}              # where HEAD was 2 moves ago
git checkout main@{yesterday}
git checkout main@{2.weeks.ago}
```

### See Your Navigation History

```bash
git reflog                # complete history of HEAD movements
```

### Quick Navigation

```bash
git checkout -            # toggle between last two positions
```

### Create Branch from Detached HEAD

```bash
git switch -c new-branch-name
```

## Checkout vs Reset: Ontological Distinction

### Checkout: Move Your View
```bash
git checkout <commit>     # detached HEAD, graph unchanged
```
Moves HEAD through immutable history. The commit graph remains unchanged; only your viewing position moves.

### Reset: Move Branch Pointer
```bash
git reset --hard <commit>    # moves branch + HEAD, updates working dir
git reset --soft <commit>    # moves branch + HEAD, keeps changes staged
git reset --mixed <commit>   # moves branch + HEAD, unstages changes (default)
```
Moves the branch reference itself, effectively rewriting that branch's history.

**Essential distinction:**
- `checkout`: traversal through immutable graph
- `reset`: mutation of branch reference (graph unchanged, reference moved)

## Ontological Foundation

### The Commit Graph Structure

Git's fundamental architecture is a **directed acyclic graph (DAG)** where:

- **Nodes:** commit objects (immutable, content-addressed by SHA-1 hash)
- **Edges:** parent relationships (child points to parent)
- **Direction:** flows backward through time (commits know their parents, not children)
- **Acyclic:** no loops (time doesn't cycle)

### References: Named Pointers

- **Branches** (`refs/heads/*`): mutable pointers to commits
- **Tags** (`refs/tags/*`): typically immutable pointers to commits
- **HEAD** (`HEAD`): special pointer indicating your current position

HEAD can point to:
1. A branch reference (normal state): `HEAD → refs/heads/main → commit`
2. A commit directly (detached HEAD): `HEAD → commit`

### Working Directory as Projection

Your working directory is the materialized state of whichever commit HEAD currently references. When you navigate history:

1. HEAD moves to target commit
2. Git reconstructs that commit's complete file tree
3. Your working directory becomes that historical state

The commit graph never changes—you're simply projecting different nodes into your working space.

## Essential Understanding

**Power through immutability:**

Git's power comes from treating history as immutable. Every commit is permanent and content-addressed. Navigation is traversal through this permanent structure, not mutation of it.

**References as ephemeral:**

Branches and HEAD are mutable pointers atop an immutable graph. They provide convenient names and navigation but don't alter the underlying structure.

**Detached HEAD as direct access:**

Detached HEAD state is git's way of saying: "You're viewing history directly, not through a branch lens." You have unmediated access to any commit. You can create new branches from this position, making any historical state your new starting point.

**The graph is the truth:**

Everything else—branches, tags, HEAD—are convenience mechanisms for navigating and organizing the fundamental commit DAG. Master the graph; master git.