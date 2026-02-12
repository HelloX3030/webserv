# Git Stash: Mechanics and Ontology

## Retrieval Commands

Apply most recent stash and remove from stack:
```bash
git stash pop
```

Apply most recent stash but keep in stack:
```bash
git stash apply
```

View all stashes:
```bash
git stash list
```

Apply specific stash:
```bash
git stash apply stash@{2}
```

## Location

Stashes exist in `.git/refs/stash` as a special reference pointing to commit objects in the object database.

## Ontology

A stash is fundamentally a **commit object** (identical in structure to any git commit) storing:

- Your working directory state
- Your staging area state
- Parent relationships

The stash reference maintains a reflog-style stack of these commits.

**Operational distinction:**

- `git stash pop`: applies changes AND removes from stack
- `git stash apply`: applies changes BUT preserves stack entry

The stack structure enables multiple stashes to coexist, each accessible via its index notation `stash@{n}`.

## Essential Understanding

Stashes aren't a separate git mechanism—they're regular commits organized in a stack data structure. The `.git/refs/stash` reference is simply a pointer managing this stack. When you stash, git creates commits; when you retrieve, git applies those commits back to your working state.

This reveals git's fundamental architecture: everything is either an object (blob, tree, commit, tag) or a reference pointing to objects. Stash leverages this substrate by creating temporary commits accessible through a dedicated reference.