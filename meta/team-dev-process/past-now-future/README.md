## purpose

This directory exists for asynchronous coordination items
that require discussion before action.

Used when:

  I spot potential issue in Lukas's code → flag for discussion, don't fix unilaterally
  Design choice needs mutual agreement → document question, discuss before implementing
  Code change would affect Lukas's current work → note instead of risking conflict
  I don't understand Lukas's approach → ask rather than assume

NOT used for:

  My own task tracking (we are using GitHub projects to handle this)
  Implementation notes for yomyur own features (these go in code comments)
  User-facing documentation (separate docs/ for that)
  Complete project history (git log provides this)

Essence: A synchronization buffer for items requiring mutual understanding before action.

## structure

### _pending

code review Qs (per branch)
upcoming design decisions
potential conflicts
critical issues (blocking merge)
minor, e.g. style (naming of files...)

updated as I work


after discussion, move to:

### dated file (yyyymmdd)

Qs discussed
decisions made
some insights
actions items with owners