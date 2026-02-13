# Branching & Integration Workflow

## Law

**Main branch represents valid program state at all times.**

Every commit in main must compile, pass tests, and be deployable.


## Principles

1. **Feature Isolation**: Each logical unit of work lives in its own branch
   - Enables independent development
   - Allows selective integration
   - Facilitates rollback if needed

2. **Remote Backup**: All work-in-progress pushed to remote
   - Protects against data loss
   - Provides visibility into each other's progress
   - No work exists only on local machine

3. **Clean History**: Linear history via rebase + fast-forward merge
   - Makes debugging via bisect possible
   - Clear causality (what led to what)
   - No merge commit noise

4. **Integration Safety**: Test before merge, never after
   - Broken code never enters main
   - Every commit in main is valid program state
   - Failures caught before integration, not after


## Branch Naming
```
<developer>/<feature-name>

Examples:
ghr/http-parser
ghr/socket-mgmt
lukas/config-parser
lukas/cgi-executor
```

**Why prefix with name**:
- Immediate visibility of who owns what
- No branch name conflicts
- Clear responsibility

**What constitutes a "feature"**:
- Single subsystem or module (http parser, config parser)
- Logical unit that can be tested independently
- Typically maps to one major requirement from spec


## Workflow

### 1. Starting New Feature
```bash
git checkout main
git pull
git checkout -b <yourname>/<feature-name>
```

**Why pull first**: Ensure you branch from latest valid state.


### 2. Development
```bash
# Work on feature
# Commit atomically (one logical change per commit)
git commit -m "implement X"

# Push to remote daily (minimum)

# 1st time pushing to new remote branch (`-u` means `--set-upstream`):
git push -u origin <yourname>/<feature-name>

# after remote branch has been set as the upstream (tracking branch) & assuming contined connection to it:
git push
```

**Why push daily**: Backup + visibility. Other person can see your progress.


### 3. Ready to Integrate

**Prerequisites**:
- Feature complete
- Tests written and passing locally
- Code reviewed (via meta/ files)

```bash
# Get latest main
git checkout main
git pull

# Rebase your feature onto latest main
git checkout <yourname>/<feature-name>
git rebase main

# Resolve any conflicts
# Test again after rebase (conflicts may break things)

# Merge to main (fast-forward only)
git checkout main
git merge <yourname>/<feature-name> --ff-only
git push

# Clean up
git branch -d <yourname>/<feature-name>
git push origin --delete <yourname>/<feature-name>
```
    GANESHA: see thread 20260212-0 for ctx here

**Why rebase before merge**:
- Moves your changes on top of latest main
- Forces you to resolve conflicts before integration
- Results in linear history

**Why --ff-only**:
- Enforces that rebase happened
- Prevents merge commits
- If it fails, you forgot to rebase


## Conflict Resolution

Conflicts mean: you both modified same code region.

**When conflict occurs during rebase**:
```bash
# Git marks conflicts in files
# Edit files, choose correct version
# Test that resolution is correct
git add <resolved-files>
git rebase --continue

# If you realize you made mistake:
git rebase --abort  # Start over
```

**Prevention**: Communicate which files you're working on (see Communication document).


## Preconditions

This workflow assumes:
1. Both developers check communication channel regularly (at least daily) 
(for this project: SimpleX)
2. Both developers know what other is working on (for this project: GitHub Projects)
3. Both developers understand that main is sacred
4. Both developers have tested locally before attempting merge


## What This Achieves

- **Safety**: Bad code never enters main
- **Clarity**: History shows what changed, when, by whom
- **Independence**: Can work on separate features without coordination overhead
- **Recoverability**: Can revert any feature cleanly
- **Simplicity**: No complex merge strategies, no pull request UI dependencies


## The Only Rule

**Never push directly to main.**

Always go through: branch → test → rebase → ff-merge → push.

One exception: Fixing critical bug in main (rare). Even then, test first.