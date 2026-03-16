# Git Branches & Upstream Relationships  
### A Structural and Mechanical Map of How Git Actually Works

---

## 1. Core Mental Model

Git is fundamentally:

> **Named pointers (refs) to commits + configuration that defines relationships between those pointers.**

A branch is nothing more than:

A named pointer to a commit.

Example:

main      → C5  
feature-x → C7  

Branches do not contain commits.  
They point to commits.

---

## 2. The Three Types of Branch References

### 2.1 Local Branches

Stored in:

.git/refs/heads/<branch-name>

Examples:

.git/refs/heads/main  
.git/refs/heads/feature-x  

These are fully under your control.

Created by:

git branch feature-x  
git checkout -b feature-x  

---

### 2.2 Remote Branches (On the Server)

These exist in the remote repository (e.g., GitHub).

Examples:

origin/main  
origin/feature-x  

You cannot modify them directly.  
They change only when you push.

---

### 2.3 Remote-Tracking Branches (Local Cache)

Stored in:

.git/refs/remotes/<remote>/<branch>

Example:

.git/refs/remotes/origin/main  

These are your local copies of the remote state.

They are updated when you run:

git fetch  
git pull  

They are read-only from your perspective.

---

## 3. What Is an Upstream Branch?

An upstream branch is:

> The remote branch your local branch is configured to track.

This relationship is stored in:

.git/config

Example configuration:

[branch "feature-x"]  
    remote = origin  
    merge = refs/heads/feature-x  

This means:

Local branch: feature-x  
Tracks:        origin/feature-x  

Upstream is configuration — not a special type of branch.

---

## 4. Setting Upstream with `-u`

When you run:

git push -u origin feature-x  

Git performs two actions:

1. Pushes feature-x to origin  
2. Sets origin/feature-x as the upstream branch  

After this, you can simply run:

git push  
git pull  

Git now knows the relationship.

---

## 5. Is `-u` a One-Time Operation?

For a given branch, typically yes.

You only need to establish the upstream relationship once.

However:

- You can change it later.
- You can overwrite it.
- Running it again does not duplicate anything.

It modifies configuration, nothing more.

---

## 6. Idempotency of `git push -u`

If you repeatedly run:

git push -u origin feature-x  

and everything already exists:

- The remote branch is updated (normal push behavior)
- The config values are set again to the same values

Nothing new is created.

No duplicate upstream branches appear.

It is idempotent.

---

## 7. Verifying Upstream Relationships

### 7.1 Quick Overview

git branch -vv  

Example output:

* feature-x  a1b2c3d [origin/feature-x] Add feature  

This shows:

- Current branch  
- Commit hash  
- Upstream branch in brackets  
- Ahead/behind status  

---

### 7.2 Inspecting Configuration Directly

git config --get branch.feature-x.remote  
git config --get branch.feature-x.merge  

This reveals the exact stored mapping.

---

## 8. When Are Branches Created?

### 8.1 Local Branch Creation

git branch feature-x  

Creates:

.git/refs/heads/feature-x  

Or:

git checkout -b feature-x  

Creates and switches.

---

### 8.2 Remote Branch Creation

A remote branch is created when you push:

git push origin feature-x  

If it doesn’t exist, Git creates it on the remote.

Important:

`-u` does NOT create the remote branch.  
The push creates it.

`-u` only sets tracking metadata locally.

---

## 9. What Happens During `git fetch`?

git fetch  

Updates:

.git/refs/remotes/origin/*  

It does NOT:

- Modify local branches  
- Merge anything  
- Change your working tree  

It only updates your knowledge of the remote state.

---

## 10. What Happens During `git pull`?

git pull  

Is equivalent to:

git fetch  
git merge (or rebase) upstream  

This only works automatically if upstream is set.

Otherwise Git does not know what to merge.

---

## 11. Multiple Local Branches Tracking the Same Remote Branch

Yes, this is possible.

Example:

local-a → origin/main  
local-b → origin/main  

Both local branches track the same upstream.

Git allows this.

However, pushing from both can cause divergence if not managed carefully.

Git does not prevent this. It assumes you understand the graph.

---

## 12. Ahead / Behind Mechanics

When you see:

[origin/feature-x: ahead 2]  

Git is comparing:

Local branch pointer  
vs  
Remote-tracking branch pointer  

It calculates divergence in the commit graph.

No magic. Just graph comparison.

---

## 13. Full Structural Layer Map

1. LOCAL REF  
   .git/refs/heads/feature-x  

2. CONFIG MAPPING  
   branch.feature-x.remote = origin  
   branch.feature-x.merge  = refs/heads/feature-x  

3. REMOTE-TRACKING REF  
   .git/refs/remotes/origin/feature-x  

4. ACTUAL REMOTE SERVER REF  
   The branch on the remote repository  

Four separate layers.

Confusing them is beginner behavior.  
Separating them clearly is mastery.

---

## 14. Key Takeaways

- A branch = pointer to a commit.  
- Upstream = configuration mapping.  
- `-u` sets tracking metadata.  
- Push creates remote branches.  
- Fetch updates remote-tracking branches.  
- Pull = fetch + merge/rebase.  
- Everything reduces to refs + config + graph comparison.  
