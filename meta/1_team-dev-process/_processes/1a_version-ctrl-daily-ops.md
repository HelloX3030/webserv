# Daily Git Operations

## Law

Stay synchronized with main. 
Never let your feature branch drift far from latest valid state.

## Branch Navigation

### Switch to existing branch
```bash
git checkout <branch-name>
```

### See all branches
```bash
git branch           # local
git branch -r        # remote
git branch -a        # both ("all")
```

Current branch marked with `*`

### Quick status check
```bash
git status           # what's changed locally
git log --oneline -5 # recent commits
```

## Mid-Development Sync

**When to sync**: Lukas merged to main, you're still developing your feature.

**Why sync**: Get his changes into your branch.
Prevents massive conflicts later. Enables using his code in your feature.

### Workflow
```bash
# Save current work if uncommitted
git add .
git commit -m "WIP: checkpoint before sync"

# Get latest main
git checkout main
git pull

# Bring main changes into your feature
git checkout <yourname>/<feature>
git rebase main

# Resolve conflicts if any
# Test that your feature still works with new main
```

**If conflicts occur**:
```bash
# Git marks conflicts in files with <<<<<<< ======= >>>>>>>
# Edit files, choose correct resolution
# Test the resolution

git add <resolved-files>
git rebase --continue

# If you realize mistake during conflict resolution:
git rebase --abort  # start over
```

**After successful rebase**:
```bash
# Your remote branch now has different history
# Force push required (safe because it's your branch)
git push --force-with-lease origin <yourname>/<feature>
```

`--force-with-lease` = safer than `--force`.
Aborts if remote has commits you don't have locally.

## Temporary Work Saving

Need to switch branches but have uncommitted changes?

```bash
# Save work temporarily
git stash

# Do other work (switch branches, etc.)

# Restore saved work
git stash pop
```

**When to use**:
- Quick context switch to help Lukas
- Pull main while you have uncommitted work
- Experiment with approach, want to revert quickly

**When NOT to use**:
- Long-term work-in-progress (use commits instead)
- As backup mechanism (commit and push instead)

## Viewing Changes

### What changed in working directory
```bash
git diff                    # unstaged changes
git diff --staged           # staged changes
git diff main               # your branch vs main
```

### What changed in commits
```bash
git log --oneline -10       # recent commits
git log --oneline main..    # commits in your branch not in main
git show <commit-hash>      # specific commit details
```

## Common Patterns

### Daily start routine
```bash
git checkout main
git pull
git checkout <yourname>/<feature>
git rebase main             # sync with any overnight merges
# Continue work
```

### Before leaving for day
```bash
git add .
git commit -m "descriptive message"
git push                    # backup + visibility
```

### Lukas just merged something
```bash
git checkout main
git pull
git checkout <yourname>/<feature>
git rebase main             # get his changes
git push --force-with-lease # update remote
# Test your feature still works
```

### Made commits but want to reorganize
```bash
git rebase -i HEAD~3        # interactive rebase last 3 commits
# Editor opens, you can: reorder, squash, reword, drop
```

## Preconditions

- Understand rebase changes history (requires force push to remote)
- Never rebase commits that exist on main (only rebase feature branches)
- Always test after rebase (conflicts may break code subtly)
- Communication protocol followed (you know what Lukas merged)

## What This Achieves

- Feature branches stay current with main
- Conflicts caught early (small, manageable)
- Can use Lukas's code immediately after merge
- Less painful final integration
- Shared understanding of project state