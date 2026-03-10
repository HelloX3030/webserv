# Rebase as Process

Rebase = replay commits one-by-one.

What happens mechanically:

git rebase main

Git does:

    Find common ancestor of your branch and main
    Collect all YOUR commits since ancestor
    Reset your branch to point at main
    Apply your commits ONE AT A TIME on top of main


When conflict occurs:

Git stops mid-process
Leaves you in "rebasing" state
You resolve conflict in that ONE commit

git rebase --continue = resume, try next commit
git rebase --abort = undo everything, return to pre-rebase state


Why step-by-step matters:

If you have commits A, B, C and B conflicts:

Git applies A successfully
Git tries B, conflicts, STOPS
You fix B
Git continues with C

If you --abort: returns to before A. Complete rollback.

State during rebase:

git status  # shows: "interactive rebase in progress"
git log     # shows partial history (confusing, don't use during rebase)

Interactive rebase (git rebase -i):

Opens editor, lets you:

    Reorder commits
    Squash commits together
    Reword commit messages
    Drop commits entirely
    Edit commits (pause rebase, modify, continue)

Each operation in editor = instruction for the step-by-step process.