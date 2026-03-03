# Git Commit Messages

## Ontology: What IS a Commit Message?

A commit message is **permanent historical documentation 
of reasoning embedded in the version control graph**.

Not:
- A description of what files changed (the diff shows this)
- A personal note to yourself (use comments for that)
    [20260214] what's meant by this? comments in code? they shouldn't be for random personal notes but also meaningful! to enable literate programs in normal editors outside of org-babel
- A status update (use project management tools)

But rather:
- **Archaeological artifact**: enables future developers (including yourself) 
to understand *why* decisions were made
- **Reasoning trace**: documents the logical necessity that drove the change
- **Historical anchor**: provides context that code alone cannot convey

### The Fundamental Problem

Code shows *what* the system does.
Comments show *how* it works. 
  [20260214] and ideally the code is so well done: clearly structured, named etc. 
  so that how is also clear. and potentially some "why" is included too in comments?
Commit messages must show *why it became necessary*.

When you examine code 6 months later and think "why did I do this?", 
the commit message is your only salvation. If it says "fixed bug" or "updates", you've failed.
  [20260214] this sounds like AI slopp. "Only salvation"? then your code quality must be shit...

## Why This Matters (Necessity)

### For Individual Work
- **Memory decay**: You will forget why you made decisions
- **Context loss**: Future-you lacks present-you's mental context
- **Debugging**: `git bisect` finds bug-introducing commit—message explains what you were trying to achieve

### For Collaboration
- **Review efficiency**: Reviewers understand intent without asking
- **Knowledge transfer**: New team members learn system evolution
- **Blame archeology**: `git blame` shows who changed line—message explains why change was necessary

### For Project History
- **Changelog generation**: Semantic commit types enable automatic changelog creation
- **Semantic versioning**: Breaking changes clearly marked
- **Historical understanding**: Trace decision evolution across months/years

## Structure: The Canonical Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Subject Line (First Line)

**Format:** `<type>(<scope>): <subject>`

**Constraints:**
- Maximum 50 characters
- Imperative mood
- No period at end
- Lowercase after colon

**Imperative mood:** Complete the sentence "If applied, this commit will ___"
- ✓ "fix buffer overflow in parser"
- ✗ "fixed buffer overflow in parser"
- ✗ "fixes buffer overflow in parser"

**Why imperative?** Git itself uses imperative ("Merge branch X", "Revert Y"). Consistency with git's own messages.

**Why 50 characters?** 
- `git log --oneline` truncates at 50
- GitHub UI truncates at 72
[20260214] I don't care about GitHub. My elite work will be self-hosted, with some group projects using codeberg.
- Forces clarity through constraint
- Scannable in terminal without wrapping


### Body (Optional but Recommended)

**Format:**
- Blank line separates subject from body
- Wrap at 72 characters
- Multiple paragraphs allowed
- Use bullet points for lists

**Content:**
- **Why** the change exists (necessity)
- **What problem** it solves
- **Trade-offs** considered
- **Alternatives** rejected and why
- **Context** that diff cannot provide

**What NOT to include:**
- How the change was implemented (diff shows this)
- Mechanics of file changes
- Trivial details obvious from code

**Example body structure:**
```
Problem statement: what was wrong, why change needed.

Solution approach: what this change accomplishes, why this
approach chosen over alternatives.

Trade-offs: what was sacrificed, what was gained.

Additional context: anything future readers need to know.
```

### Footer (Optional)

**Breaking changes:**
```
BREAKING CHANGE: description of what breaks and migration path
```

**Issue references:**
```
Refs: #123
Fixes: #456
Closes: #789
```

**Co-authors:**
```
Co-authored-by: Name <email@example.com>
```

## Types: Semantic Categorization

### Essential Types (Conventional Commits Specification)

**feat**: New functionality
- User-facing feature
- API addition
- New capability
- Example: `feat(http): add POST method support`

**fix**: Bug correction
- Something was broken, now works
- Behavior correction
- Example: `fix(parser): prevent buffer overflow on malformed headers`

**docs**: Documentation
- README updates
- Code comments
- Specification documents
- API documentation
- Example: `docs(spec): restructure requirements by domain`

**refactor**: Code restructure
- Same external behavior
- Different internal implementation
- Architectural improvement
- Example: `refactor(server): extract connection state machine`

**test**: Test additions/modifications
- Unit tests
- Integration tests
- Test infrastructure
- Example: `test(http): add edge cases for chunked encoding`

**build**: Build system changes
- Makefile modifications
- Compiler flags
- Dependency updates
- Example: `build: add debug target with sanitizers`

**chore**: Maintenance tasks
- Dependency updates
- Tooling changes
- No production code changes
- Example: `chore: update .gitignore for build artifacts`

**perf**: Performance improvements
- Optimization
- Efficiency gains
- Measurable improvement
- Example: `perf(parser): use zero-copy string_view for headers`

**style**: Code style changes
- Formatting
- Whitespace
- No logic changes
- Example: `style: apply clang-format to codebase`

**ci**: CI/CD changes
- GitHub Actions
- Build pipelines
- Automation
- Example: `ci: add automated testing on pull requests`

**revert**: Revert previous commit
- Reverts specific commit
- Explain why revert needed
- Example: `revert: "feat(http): add HTTP/2 support"`


### Extended Types (Domain-Specific)

For specialized projects, define additional types:
- `security`: Security fixes
- `deps`: Dependency updates
- `config`: Configuration changes
- `api`: API-specific changes


### Type Selection Decision Tree

```
Does this add new user-facing functionality?
  → feat

Does this fix incorrect behavior?
  → fix

Does this change only documentation?
  → docs

Does behavior stay identical but implementation changes?
  → refactor

Does this improve performance measurably?
  → perf

Does this add/modify tests?
  → test

Does this change build system?
  → build

None of the above?
  → chore
```



## Scope: Where in System

Scope identifies the module/component/domain affected.

**Format:** `type(scope): subject`

**Examples by project type:**

**Web server (webserv):**
- `feat(http-parser): add chunked encoding support`
- `fix(cgi): prevent zombie processes`
- `refactor(event-loop): extract poll wrapper`
- `docs(config): document location directive syntax`

**Library:**
- `feat(api): add async support to client`
- `fix(auth): validate token expiration correctly`
- `test(crypto): add test vectors for SHA-256`

**Multi-component system:**
- `feat(frontend): add dark mode toggle`
- `fix(backend): prevent SQL injection in user query`
- `refactor(database): normalize user table schema`

**When to omit scope:**
- Change affects entire system
- No clear module boundary
- Cross-cutting concern

**Example:** `build: upgrade to C++17 standard`



## Examples: Elite vs Amateur

### Example 1: HTTP Parser Buffer Fix

**Amateur:**
```
fixed bug
```

**Mediocre:**
```
fix: buffer overflow in HTTP parser
```

**Good:**
```
fix(http-parser): prevent buffer overflow on oversized headers

strcpy allowed unbounded client input, risking memory corruption.
Replaced with strncpy with explicit buffer size check.

Maximum header size now enforced at 8KB per RFC 2616 guidance.
Requests exceeding limit receive 431 Request Header Fields Too Large.
```

**Why elite:**
- Problem stated (unbounded input)
- Solution explained (strncpy + size check)
- Design decision documented (8KB limit from RFC)
- HTTP compliance noted (431 status code)

### Example 2: Refactoring Event Loop

**Amateur:**
```
refactoring
```

**Mediocre:**
```
refactor: clean up event loop
```

**Good:**
```
refactor(event-loop): extract connection state machine

Event loop contained 300-line switch statement handling all
connection states, making logic difficult to test and modify.

Extracted state machine into separate Connection class with
explicit state transitions. Each state now testable in isolation.

No behavioral changes. All existing tests pass.

Enables future work: easier to add keep-alive support with
explicit state tracking.
```

**Why elite:**
- Problem (300-line switch, hard to test)
- Solution (extract to class)
- Verification (no behavioral change, tests pass)
- Future benefit (enables keep-alive)

### Example 3: Adding Feature

**Amateur:**
```
added POST
```

**Mediocre:**
```
feat: implement POST method
```

**Good:**
```
feat(http): add POST method with body handling

Implements POST per RFC 2616 section 9.5.

Request body reading controlled by Content-Length header.
Maximum body size enforced via configuration (default 1MB).
Requests exceeding limit receive 413 Payload Too Large.

File upload support included: multipart/form-data boundary
parsing with temporary file storage in configured directory.

Tested with:
- curl POST with JSON body
- HTML form file upload
- Browser fetch() API
- 1000 concurrent POST requests (no memory leaks)

Closes #42
```

**Why elite:**
- RFC compliance specified
- Design decisions documented (max size, storage)
- Testing methodology described
- Issue reference included

### Example 4: Breaking Change

**Format for breaking changes:**

```
feat(api): redesign configuration file format

BREAKING CHANGE: Configuration syntax changed from JSON to
NGINX-style blocks. Migration path:

Old format:
{
  "server": {
    "port": 8080,
    "root": "/var/www"
  }
}

New format:
server {
  listen 8080;
  root /var/www;
}

Migration script provided: scripts/migrate_config.sh

Rationale: NGINX syntax more expressive for location blocks
and nested directives. JSON format could not represent route
precedence clearly.

All example configs updated. Documentation updated.

BREAKING CHANGE must appear in footer or body.
```

## Body Content: What to Include

### Always Include

**Problem statement:**
- What was wrong or missing
- Why change was necessary
- Impact if not fixed

**Solution approach:**
- What this change accomplishes
- Why this approach chosen

### Often Include

**Trade-offs:**
- What was sacrificed for what gain
- Performance vs readability
- Simplicity vs features

**Alternatives considered:**
- What other approaches were possible
- Why they were rejected

**Testing:**
- How you verified correctness
- Edge cases covered
- Performance measurements

### Sometimes Include

**Context:**
- Historical background
- Related discussions
- Design philosophy

**Migration notes:**
- How to adapt to breaking changes
- What users must do

**Future work:**
- What this enables
- What remains to be done


### Never Include

**Mechanics:**
- "Changed line 42 from X to Y"
- "Added function foo()"
- File-by-file change description

**Apologies or uncertainty:**
- "I think this might fix..."
- "Sorry for the mess"
- "This probably works"

**Personal notes:**
- "Finally got this working!"
- "This was hard"
- Emotional content



## Message Length Guidelines


### Subject Line
- Target: 50 characters
- Hard limit: 72 characters
- Beyond 72: gets truncated in most tools


### Body
- Wrap at 72 characters
- Blank line between paragraphs
- No hard limit on total length
- If > 20 lines: consider if commit too large


### When to Write Long Messages

**Write extensively when:**
- Complex problem required complex solution
- Breaking change needs migration guide
- Design decision had multiple alternatives
- Future maintainers will need context

**Write briefly when:**
- Trivial fix (typo, formatting)
- Self-explanatory change
- Following established pattern



## Common Patterns

### Fixing Bugs

```
fix(module): prevent <problem> when <condition>

<problem> occurred because <root cause>.
Manifested as <symptoms>.

Fixed by <solution approach>.

Reproducer: <steps to trigger bug>
Verification: <how fix was tested>

Fixes #<issue>
```

### Adding Features

```
feat(module): add <capability>

Implements <specification/requirement>.

Use case: <why users need this>

Design decisions:
- <choice 1>: <rationale>
- <choice 2>: <rationale>

Testing: <verification approach>

Closes #<issue>
```

### Refactoring

```
refactor(module): <restructure description>

Current implementation: <problems with existing code>

New implementation: <improvements>

No behavioral changes. All tests pass.

Enables: <future work this makes easier>
```

### Performance Improvements

```
perf(module): optimize <operation>

Before: <measurement>
After: <measurement>
Improvement: <percentage or factor>

Approach: <what was optimized and how>

Benchmark: <how measured>
Trade-offs: <what complexity was added, if any>
```



## Anti-Patterns: What NOT to Do

### Vague Subjects

❌ `fix: bug`  
❌ `update: stuff`  
❌ `refactor: improvements`  
❌ `chore: changes`


### Missing Context

```
fix: null pointer check
```

**Problem:** What null pointer? Where? Why was it null? What triggers it?

### Change Description Instead of Reasoning

```
feat: add error handling

Added try-catch blocks to main functions.
Updated error messages.
Modified logging.
```

**Problem:** Describes mechanics (what diff already shows) instead of explaining why error handling was needed and what problem it solves.

### Multiple Unrelated Changes

```
feat: add POST support and fix memory leak and update docs
```

**Problem:** One commit should be one logical change. Split into three commits.

### Humor or Informality

❌ `fix: oops, forgot semicolon lol`  
❌ `feat: finally got this working after 3 days!!!`  
❌ `refactor: cleanup because code was trash`

### Assuming Context

```
fix: handle the case we discussed
```

**Problem:** What case? Discussed where? Commit message must be self-contained.

## Advanced Techniques

### Atomic Commits

**Principle:** Each commit represents one logical change, completely.

**Why:** 
- `git revert` can cleanly undo the change
- `git bisect` can identify bug-introducing commit
- Each commit in history is understandable independently

**How:**
- Stage related changes together: `git add -p`
- Commit frequently (small logical units)
- Amend if forgot something: `git commit --amend`

### Commit Message Templates

Create `.gitmessage` template:

```
# <type>(<scope>): <subject>

# Why this change exists:

# What problem it solves:

# Trade-offs considered:

# --- COMMIT END ---
# Type can be: feat, fix, docs, refactor, test, build, chore, perf
# Subject: imperative mood, max 50 chars
# Body: wrap at 72 chars
# Footer: Refs #<issue>, BREAKING CHANGE, Co-authored-by
```

Configure git to use it:
```bash
git config --global commit.template ~/.gitmessage
```

### Conventional Commits Tooling

**commitlint:** Enforce commit message format
```bash
npm install -g @commitlint/cli @commitlint/config-conventional
```

**commitizen:** Interactive commit message builder
```bash
npm install -g commitizen
git cz  # instead of git commit
```

**standard-version:** Auto-generate changelog
```bash
npm install -g standard-version
standard-version  # generates CHANGELOG.md
```

## Integration with Workflow

### During Development

```bash
# Make atomic changes
git add -p  # stage only related changes

# Write thoughtful message
git commit  # opens editor for full message

# Or for trivial changes
git commit -m "fix(typo): correct variable name in comment"
```

### During Review

Reviewers examine commit messages to understand:
- Intent behind changes
- Why approach chosen
- What problem being solved

Well-written messages reduce review round-trips.

### During Maintenance

```bash
# Find when bug introduced
git bisect start
git bisect bad HEAD
git bisect good v1.0.0
# Git checks out commits, you test
# Bisect finds commit, message explains what author intended

# Understand why line changed
git blame file.cpp
# See commit hash, read message for reasoning

# Generate changelog
git log --oneline --grep="^feat"
git log --oneline --grep="^fix"
```

## Historical Context: Where These Practices Originated

### Linux Kernel
- Linus Torvalds established many conventions
- 50/72 rule from email constraints (80-column terminals)
- Imperative mood from patch submission culture
- Detailed bodies from mailing list review process

### AngularJS Convention (2013)
- Introduced `<type>(<scope>): <subject>` format
- Semantic types (feat, fix, docs, etc.)
- Enabled automated changelog generation
- Became basis for Conventional Commits

### Conventional Commits Specification (2018)
- Formalized AngularJS convention
- Machine-readable commit format
- Semantic versioning integration
- Industry-wide adoption (GitHub, GitLab, npm)

### Modern Evolution
- Tooling ecosystem (commitlint, commitizen, semantic-release)
- CI/CD integration
- Automated release management
- Monorepo conventions (scope for package/module)

## Further Study

### Primary Sources

**Conventional Commits Specification:**  
https://www.conventionalcommits.org/

**Chris Beams: "How to Write a Git Commit Message" (2014):**  
https://chris.beams.io/posts/git-commit/  
Classic article establishing many conventions.

**Linux Kernel Documentation:**  
https://www.kernel.org/doc/html/latest/process/submitting-patches.html  
Gold standard for commit message practices.

**AngularJS Commit Guidelines (Historical):**  
https://github.com/angular/angular.js/blob/master/DEVELOPERS.md#commits  
Original source of type-scope format.

### Elite Repositories to Study

**Linux Kernel:**  
https://github.com/torvalds/linux  
Examine commits from Linus Torvalds and maintainers.

**Git itself:**  
https://github.com/git/git  
Meta: study git's commit history to learn git practices.

**Rust:**  
https://github.com/rust-lang/rust  
Excellent commit hygiene, thorough messages.

**LLVM:**  
https://github.com/llvm/llvm-project  
Clear, technical, precise commit messages.

### Tools

**commitlint:** https://commitlint.js.org/  
**commitizen:** https://github.com/commitizen/cz-cli  
**semantic-release:** https://semantic-release.gitbook.io/  
**conventional-changelog:** https://github.com/conventional-changelog/conventional-changelog

## Principles Summary

### The Five Laws

1. **Subject line completes: "If applied, this commit will ___"**
2. **Body explains WHY, not WHAT** (diff shows what)
3. **One commit = one logical change** (atomic)
4. **Messages are for future archeologists** (including yourself)
5. **Clarity over brevity** (when in doubt, write more context)

### The Ultimate Test

Read your commit message 6 months from now:
- Can you understand what problem existed?
- Can you understand why this solution was chosen?
- Can you understand what trade-offs were made?

If no to any question: message needs improvement.

### From Necessity

Every convention in this document derives from necessity:
- 50-char limit: terminal width constraints
- Imperative mood: consistency with git's own messages
- Semantic types: enable tooling and automation
- Detailed bodies: code cannot document its own reasoning
- Breaking change markers: users need migration guidance

Nothing is arbitrary. Everything serves future understanding.

## Final Note

Writing elite commit messages is not about following rules—it's about **respecting future readers** (including yourself). 

Every commit is a permanent artifact in your project's history. Invest the 60 seconds to document your reasoning. Future-you will thank present-you.