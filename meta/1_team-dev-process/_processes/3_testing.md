# Testing Strategy

## Law

Main branch must always pass all tests.

If main fails tests, fixing it becomes highest priority.

## Principles

1. Test before merge, never after
   - Broken code never enters main
   - Other person can always pull and continue working
   - Every commit in main represents valid program state

2. Tests define correctness
   - Specification is ambiguous, tests are concrete
   - Tests show what "working" means
   - Disagreement about behavior → write test showing expected behavior → discuss

3. Fast feedback
   - Tests run in seconds, not minutes
   - Can test frequently during development
   - No excuse to skip testing

4. Shared responsibility
   - Both developers run tests before merge
   - Both developers add tests for features they implement
   - Both developers fix tests they break

## Workflow Integration

Before merging to main:
```
make test
```

If tests pass → merge.
If tests fail → fix, then merge.

No exceptions.

## Test Scope

Three levels, run in order:

Unit: Individual functions in isolation
Integration: Components working together  
System: Complete server behavior via real requests

Each level catches different failures. All three required before merge.

## Coordination Aspect

When you merge to main:
- Tests passed on your machine
- Other person pulls your changes
- Other person runs tests on their machine
- Tests still pass

This creates trust. Main is always safe to pull.

When tests fail in main:
- Stop all other work
- Identify what broke
- Fix immediately or revert the merge
- Resume normal work only after main passes tests again

## Test Organization
```
test/unit/       - Fast, many, focused
test/integration/ - Medium speed, key interactions
test/system/     - Slower, complete scenarios
```

Makefile provides: make test

Runs all three levels. Single command, clear pass/fail.

## What This Achieves

Maintains the fundamental invariant: main always works.

Without this:
- Can't trust main
- Can't pull without fear
- Can't parallelize work
- Integration becomes painful

With this:
- Pull anytime, it works
- Work independently, integrate safely
- Find problems early, fix cheaply
- Build confidence over time

## Preconditions

Both developers:
- Understand tests are mandatory
- Run make test before every merge
- Add tests for new features
- Fix broken tests immediately

## The Only Rule

Never merge to main without passing tests.

Better to delay merge than break main.