## main principle:

Each commit represents exactly one logical change.


## Principles

Atomicity: A commit does one thing and does it completely

  Can be reverted without side effects
  Can be understood in isolation
  Can be tested independently


Clarity: Commit message answers "what" and "why", code shows "how"

  Subject line: imperative mood, <50 chars:
    "Fix buffer overflow in HTTP parser"
  
  Body: explain reasoning, not mechanics: 
    "strcpy allows unbounded input, risking overflow. 
    strncpy with explicit bounds enforces safety."


Reviewability: Changes are understandable to others (including future you)

  Diff is focused, not scattered
  No "while I was here" changes
  No "oops forgot to commit" fixups (squash these)


Bisectability: Each commit compiles and passes tests

  git bisect can find bug-introducing commits
  Every commit in main is a valid program state


## example workflow

```bash
# You're working on HTTP parser
# You notice: typo in config parser, unused variable in socket handler, HTTP parser bug

# WRONG approach:
git add -A
git commit -m "various fixes"

# CORRECT approach:
# Fix 1: HTTP parser (your actual work)
git add src/http_parser.cpp
git commit
"Fix buffer overflow in HTTP request parsing

strcpy allowed unbounded input from client. Replace with 
strncpy and explicit buffer size check. Prevents potential
memory corruption from malicious requests."

# Fix 2: Typo in unrelated file
git add src/config_parser.cpp
git commit -m "Fix typo in config parser error message"

# Fix 3: Unused variable
git add src/socket_handler.cpp  
git commit -m "Remove unused variable 'timeout' in socket handler"

# If you've already mixed changes in working directory:
git add -p src/http_parser.cpp
# Git shows each hunk, you choose: y (stage), n (skip), s (split)
# Stage only HTTP parser changes
git commit -m "Fix buffer overflow in HTTP parser"

git add -p src/config_parser.cpp  
# Stage typo fix
git commit -m "Fix typo in config parser"

# etc.
```