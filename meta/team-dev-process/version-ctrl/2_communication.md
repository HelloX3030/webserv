# Communication Protocol

## Law

**Coordinate intent before action. Notify completion after action.**

Prevents duplicate work, conflicting changes, and integration surprises.

## Principles

1. **Asynchronous by default**: Most communication doesn't require immediate response
   - Allows focused work time
   - Respects different working rhythms
   - Written record over verbal promises

2. **Synchronous when necessary**: High-bandwidth discussions require real-time
   - Complex design decisions
   - Resolving misunderstandings
   - Teaching/learning new concepts

3. **Written over verbal**: Decisions must be recorded
   - Memory fails
   - Provides future reference
   - Creates accountability

4. **Explicit over assumed**: Never assume the other person knows
   - What you're working on
   - What you changed
   - What broke

## Information Flow

### 1. Intent Declaration (before starting work)

**What to communicate**:
- Which feature/module you're starting
- Which files/subsystems you'll modify
- Expected duration (hours? days?)
- Non-trivial design approach

**When**: Before creating feature branch

**Where**: Communication channel

**Latency requirement**: Same day (prevents duplicate work)

**Example**:
```
Working on HTTP request parser today.
Files: src/http_parser.cpp, include/http.hpp
Approach: State machine for parsing, std::string_view for zero-copy
Duration: ~2 days
```

**Why this matters**: Lukas knows not to start HTTP parser. Lukas knows to avoid modifying http.hpp.

### 2. Work-in-Progress Status (during work)

**What to communicate**:
- Daily progress update (brief)
- Blockers encountered
- Questions that arose
- Unexpected complications

**When**: End of work day (or when blocked)

**Where**: Communication channel

**Latency requirement**: Daily

**Example**:
```
HTTP parser: basic GET parsing working, POST next.
Blocked: need to understand chunked encoding spec better.
Question: should we support Transfer-Encoding: gzip?
```

**Why this matters**: Keeps other person informed. Allows help with blockers. Enables course correction.

### 3. Integration Notification (after merge to main)

**What to communicate**:
- What you merged
- What changed (brief)
- How to test it
- Breaking changes to interfaces

**When**: Immediately after pushing to main

**Where**: Communication channel

**Latency requirement**: Immediate

**Example**:
```
Merged ganesha/http-parser to main.
- Added HTTPRequest class
- Supports GET, POST methods
- Test: ./webserv test.conf, curl localhost:8080/test.html
Breaking: HTTPRequest constructor signature changed
```

**Why this matters**: Other person must pull and rebase ASAP. Knows what to test. Aware of breaking changes.

### 4. Design Discussion (architecture decisions)

**What to communicate**:
- Choice between approaches (X vs Y)
- Interface design questions
- Module boundary decisions
- Trade-off analysis

**When**: Before implementation (when uncertainty exists)

**Where**: 
- Simple question: communication channel
- Complex discussion: meta/_pending.md → schedule sync

**Latency requirement**: Before implementation starts

**Example**:
```
Design question: CGI execution model
Option A: fork() - simple isolation, high overhead
Option B: thread - lower overhead, complex isolation
Need to discuss trade-offs. Sync tomorrow 14:00?
```

**Why this matters**: Prevents wasted work implementing wrong approach. Ensures mutual understanding.

### 5. Code Review (quality/correctness)

**What to communicate**:
- Logic errors found
- Style inconsistencies
- Missing edge cases
- Security concerns

**When**: After reviewing other's branch, before they merge

**Where**: meta/_pending.md (for items needing discussion)

**Latency requirement**: Before merge to main

**Example**:
```
In meta/_pending.md:

## Code Review: lukas/config-parser

Branch: lukas/config-parser
Commit: abc123f

### Line 142: Buffer overflow risk
strcpy without bounds checking. Use strncpy or std::string.

### Line 89: Memory leak
Parser allocated but never freed in error path.
```

**Why this matters**: Catches bugs before they enter main. Improves code quality. Knowledge transfer.

## Communication Etiquette

### Daily Practices

- Check communication channel first thing each work day
- Check GitHub Projects to see what other is working on
- Respond to questions within same work day
- Push branches daily (backup + visibility)

### Integration Practices

- Pull main before starting work each day
- Never push directly to main without testing
- Notify immediately after merging to main
- Pull and rebase your feature branch after other person merges

### Conflict Prevention

- Declare intent before modifying shared files
- If both need to modify same file: coordinate who goes first
- If you see other person working on related code: discuss interface first

### Rebase Notifications

**Before rebase**: If you expect conflicts
```
Rebasing ganesha/socket-mgmt onto main.
Conflicts expected in src/server.cpp (we both modified).
Will resolve and test, then ping for review.
```

**After merge**: Always
```
Merged ganesha/socket-mgmt to main.
Added epoll-based event loop.
Test: ./webserv conf/test.conf, check multiple concurrent connections.
```

## Meta Directory Usage

See meta/ structure:
- `meta/past-now-future/_pending.md`: Items for next discussion
- `meta/past-now-future/YYYY-MM-DD.md`: Record of discussions and decisions

**Add to _pending.md when**:
- You spot issue in other's code but don't want to fix unilaterally
- Design choice needs mutual agreement
- You don't understand other's approach

**Create dated file after**:
- Synchronous discussion (call, in-person, chat session)
- Document: questions asked → answers given → decisions made

## Preconditions

This protocol assumes:
1. Both developers check communication channel at least daily
2. Both developers check GitHub Projects daily
3. Both developers understand written decisions are binding
4. Both developers prioritize team coordination over individual speed

## What This Achieves

- **Prevents conflicts**: Both know what other is doing
- **Enables async work**: Don't need to be online simultaneously
- **Creates record**: Decisions documented, not lost
- **Builds trust**: Explicit communication reduces anxiety
- **Maintains quality**: Review happens before merge, not after