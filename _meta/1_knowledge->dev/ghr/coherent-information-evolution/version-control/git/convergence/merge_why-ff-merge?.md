Why Fast-Forward Merge?

What is a merge?
Merge = combining two divergent histories into one.

## 3 types of merge:

### Fast-forward (ff): 

Main simply moves forward to point at your feature branch tip. No merge commit created.

Only possible when: main hasn't changed since you branched
History stays linear: A -> B -> C -> D


### Merge commit (no-ff): Creates explicit merge commit even if ff possible.

Forces branch visibility in history
History shows: A -> B -> [merge C+D] -> E


### True merge (automatic no-ff): Required when both branches have new commits.

Main advanced while you worked
Git creates merge commit to combine changes



## Why these docs mandate ff-only?

Logical necessity: If ff-merge fails, main changed while you worked = you're out of sync.
This protocol says: rebase first, THEN merge.

Rebase = rewrite your commits as if you started from current main
After rebase: main unchanged since your "new" branch point
Therefore: ff-merge becomes possible
Result: linear history, no merge commits


What ff-only achieves:

Linear history (easier to read, bisect, understand)
Enforces discipline: must sync before integrate
Prevents "oops I merged without testing against latest main"