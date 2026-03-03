ctx:
WebServ
20260212-0_git

# Git Internals: Architecture, Filesystem Interface, and the .git Database

## The Fundamental Insight

**Git doesn't "interface" with your editor or shell.** Git interfaces with the **filesystem**—one of the fundamental layers in computing infrastructure.

```
Git ──writes──> Filesystem <──reads── All Other Tools
```

This architecture enables:
- Universal compatibility (any tool that understands files works with git)
- Decoupling (git and editor are separate processes)
- Simplicity (no git plugins needed for every tool)
- Robustness (crash-safe, tools remain functional)

## The .git Directory: A Complete Database System

When you run `git init`, git creates `.git/`—a self-contained database storing:
- Complete project history
- All file versions ever committed
- Branch and tag references
- Configuration
- Operational state

### .git Directory Structure

```
.git/
├── HEAD                 # Current branch or commit reference
├── config               # Repository configuration
├── description          # Repository description (for gitweb)
├── hooks/               # Client/server-side hook scripts
├── info/                # Global excludes, refs
│   └── exclude
├── objects/             # Content-addressed object database
│   ├── [00-ff]/         # First 2 chars of SHA-1 hash
│   ├── info/
│   └── pack/            # Packed objects for efficiency
├── refs/                # References (branches, tags)
│   ├── heads/           # Branch references
│   ├── tags/            # Tag references
│   └── remotes/         # Remote-tracking branches
├── logs/                # Reflog: history of reference movements
│   ├── HEAD
│   └── refs/
├── index                # Staging area (binary file)
└── COMMIT_EDITMSG       # Last commit message
```

This is indeed **a mini filesystem within the filesystem**—a specialized database designed for version control.

## The Object Database: Content-Addressed Storage

### Four Object Types

Git stores everything as objects in `.git/objects/`. All objects are:
- **Content-addressed** (filename = SHA-1 hash of content)
- **Immutable** (once created, never modified)
- **Compressed** (zlib compression)

#### 1. Blob Objects (File Contents)

Stores raw file data.

```bash
# Create blob
echo "Hello, World" | git hash-object -w --stdin
# Returns: 8ab686eafeb1f44702738c8b0f24f2567c36da6d

# Stored in:
.git/objects/8a/b686eafeb1f44702738c8b0f24f2567c36da6d

# Retrieve content
git cat-file -p 8ab686ea
# Hello, World
```

**Structure:** Just the file content. No filename, no metadata.

#### 2. Tree Objects (Directory Structure)

Stores directory listings: filenames + permissions + blob/tree references.

```bash
git cat-file -p main^{tree}
# 100644 blob a3f5c89...  README.md
# 040000 tree d4e8f3a...  src/
# 100755 blob b2c7e9f...  build.sh
```

**Structure:**
```
<mode> <type> <hash> <name>
<mode> <type> <hash> <name>
...
```

Trees can reference other trees (subdirectories) or blobs (files).

#### 3. Commit Objects (Snapshots)

Stores project snapshot + metadata.

```bash
git cat-file -p HEAD
# tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904
# parent a3f5c8942e4f82d7e098765bcdef4567890abcde
# author Name <email> 1234567890 +0000
# committer Name <email> 1234567890 +0000
#
# Commit message
```

**Structure:**
- Reference to tree object (complete directory state)
- Parent commit hash(es) (0 for initial, 1 normally, 2+ for merges)
- Author + committer metadata
- Commit message

#### 4. Tag Objects (Annotated Tags)

Stores permanent reference to commit + metadata.

```bash
git cat-file -p v1.0
# object a3f5c8942e4f82d7e098765bcdef4567890abcde
# type commit
# tag v1.0
# tagger Name <email> 1234567890 +0000
#
# Tag message
```

### Content-Addressed Storage Properties

**Deduplication:** Identical content → identical hash → stored once.

If you have the same file in 100 commits, git stores it once.

**Integrity:** Hash is cryptographic fingerprint. Corruption detectable immediately.

**Immutability:** Cannot change object without changing its hash (and all referencing objects).

**Efficiency:** Objects are compressed. Similar content can be delta-encoded.

## HEAD: The Current Position Pointer

### HEAD Structure

`HEAD` is a file in `.git/HEAD` containing either:

**Symbolic reference (normal state):**
```bash
cat .git/HEAD
# ref: refs/heads/main
```

HEAD points to a branch, which points to a commit.

```
HEAD → refs/heads/main → commit-object
```

**Direct reference (detached HEAD):**
```bash
cat .git/HEAD
# a3f5c8942e4f82d7e098765bcdef4567890abcde
```

HEAD points directly to a commit.

```
HEAD → commit-object
```

### What HEAD Controls

**Working directory state:** The commit HEAD references determines what files appear in your working directory.

**Next commit's parent:** When you commit, new commit's parent = current HEAD.

**Branch updates:** When HEAD points to branch and you commit, git moves the branch reference forward.

```
Before commit:
HEAD → refs/heads/main → commit-A

After commit:
HEAD → refs/heads/main → commit-B (parent: commit-A)
```

## References: Named Pointers

### Refs Directory

```
.git/refs/
├── heads/              # Local branches
│   ├── main
│   └── feature-branch
├── tags/               # Tags
│   └── v1.0
└── remotes/            # Remote-tracking branches
    └── origin/
        ├── main
        └── feature-branch
```

Each file contains a single line: the commit hash it points to.

```bash
cat .git/refs/heads/main
# a3f5c8942e4f82d7e098765bcdef4567890abcde
```

### Branch as Pointer

A branch is just a file containing a hash. Moving a branch = writing new hash to file.

```bash
# Create branch
echo "a3f5c8942e4f82d7e098765bcdef4567890abcde" > .git/refs/heads/new-branch

# Equivalent to:
git branch new-branch a3f5c89
```

**Essential understanding:** Branches are lightweight. Creating 1000 branches costs 1000 × 41 bytes (hash + newline).

## The Three Trees (Layers)

Git maintains three distinct states:

### 1. Working Directory

Filesystem representation of one commit. Where you edit files.

**Location:** Your project directory (outside `.git/`)

**Mutable:** You can modify files freely.

### 2. Staging Area (Index)

Proposed next commit. Intermediate layer.

**Location:** `.git/index` (binary file)

**Mutable:** You add/remove files with `git add` / `git restore --staged`

**Structure:** List of files with blob hashes and metadata.

```bash
git ls-files --stage
# 100644 a3f5c89... 0  README.md
# 100644 d4e8f3a... 0  src/main.c
```

### 3. Repository (Commit History)

Immutable commit graph in object database.

**Location:** `.git/objects/`

**Immutable:** Once committed, never changed.

## The Cascade: Git → Filesystem → Tools

### When You Execute `git switch main`

**1. Git Internal Operations:**
```
Read .git/HEAD → refs/heads/main → commit-hash
Read commit object → tree-hash
Read tree object → list of blobs and trees
```

**2. Git Filesystem Operations:**
```
For each file in new tree:
  If file differs from working directory:
    write(filename, blob-content)

For each file in old tree not in new tree:
  unlink(filename)
```

Git performs standard filesystem syscalls: `open()`, `write()`, `unlink()`, `mkdir()`, `rmdir()`.

**3. Filesystem Generates Events:**

Modern OSes provide filesystem notification mechanisms:
- **Linux:** inotify
- **macOS:** FSEvents
- **Windows:** ReadDirectoryChangesW
- **BSD:** kqueue

When git writes files, kernel generates events.

**4. Tools Receive Notifications:**

Your editor (Emacs, vim, VS Code) watches filesystem via these mechanisms.

**5. Editor Reacts:**

```
Receive: "File changed: /path/to/file.txt"
Action: Reload buffer from disk
Result: Buffer content updates
```

### The Key: Decoupling

```
Git Process (separate)
    ↓ writes to
Filesystem (OS kernel, shared state)
    ↑ reads from
Editor Process (separate)
```

Git and editor never communicate directly. They communicate through filesystem as shared medium.

**This is Unix philosophy:** Tools communicate via filesystem, not direct IPC.

## Emacs and Magit

### Emacs Filesystem Watching

Emacs has multiple mechanisms for detecting file changes:

**1. Auto-revert-mode:**
```elisp
(global-auto-revert-mode 1)
```
Polls filesystem, checks modification timestamps periodically.

**2. File notifications:**
```elisp
(setq auto-revert-use-notify t)
```
Uses OS-level filesystem watchers (inotify/FSEvents).

**3. Buffer hooks:**
Checks on save, on focus change, etc.

When git modifies files, Emacs detects via these mechanisms and reloads buffers.

### Magit: Git Porcelain in Emacs

[Magit](https://magit.vc/) is a git interface for Emacs. It doesn't change how git works—it's a sophisticated UI layer.

**What Magit does:**
- Parses `.git/` directory to show repository state
- Calls git commands via subprocess
- Displays results in Emacs buffers with keybindings
- Updates automatically when `.git/` changes

**How Magit helps:**

1. **Visibility:** Shows staging area, diffs, branch structure visually
2. **Efficiency:** Complex git operations via single keys
3. **Learning:** Makes git's internal state visible and explorable

**Example workflow:**
```
Open magit-status (C-x g)
See: staged files, unstaged files, recent commits, stashes
Stage chunk: move cursor to hunk, press 's'
Commit: press 'c c', write message, press 'C-c C-c'
Push: press 'P p'
```

Behind the scenes, Magit is calling `git add`, `git commit`, `git push`—same as command line.

**Pedagogical value:**

Magit makes git's internal state visible. You see:
- Exact contents of staging area
- Branch pointer positions
- Reflog entries
- Stash stack

This helps build mental model of git's architecture.

### Do Elite Developers Master These Fundamentals?

**Yes, typically.**

Elite developers in projects like:
- **DynamicLand** (Bret Victor): Deep systems understanding
- **betrusted** (bunnie huang): Hardware to software stack mastery
- **Genode** (microkernel OS): Complete stack knowledge
- **GNUnet** (distributed systems): Network to crypto to OS
- **Nym** (privacy tech): Math to implementation
- **Darkfi** (crypto-anarchy): Cryptography to p2p networks

...tend to have deep understanding of:

1. **Filesystems:** Because they're fundamental OS abstraction
2. **Git:** Because it's universal collaboration tool + good architecture example
3. **Emacs/vim:** Because powerful editors amplify capability

**Why?**

These tools touch fundamental layers:
- Filesystem: OS interface boundary
- Git: Distributed database + content-addressing
- Emacs: Programmable environment + Lisp runtime

Mastery enables:
- Building systems from first principles
- Debugging at any abstraction level
- Understanding how components compose

**Your trajectory aligns with this.**

Mastering filesystem → git → Emacs → formal verification → systems programming gives you complete vertical integration from hardware to mathematics.

## Essential Understanding

### Git Goes Deep

Git interfaces with filesystem—one of the most fundamental abstractions in computing. This gives git:

- **Universality:** Works with any tool that understands files
- **Simplicity:** No complex IPC or plugin systems
- **Robustness:** Crash-safe, tools remain functional
- **Composability:** Each tool does one thing, filesystem mediates

### .git as Database

The `.git/` directory is a complete database system implementing:

- **Content-addressed storage:** Objects identified by hash
- **Immutability:** History never changes
- **Compression:** Efficient storage via zlib + delta encoding
- **Referential integrity:** Hash chains prevent corruption
- **ACID properties:** Operations are atomic

This architecture predates "blockchain" but shares key properties: content-addressing, immutability, hash chains, distributed replication.

### Working Directory as Projection

Your working directory is a **materialization** of one commit's tree structure. When HEAD moves:

1. Git selects new commit
2. Reads tree structure from object database
3. Projects tree into filesystem
4. All tools see updated filesystem
5. Tools react to changes

The commit graph is the truth. The working directory is an ephemeral projection for human interaction.

### The Power of Fundamentals

Understanding git's architecture reveals:
- How distributed systems work (content-addressing, hash chains)
- How databases work (object storage, referential integrity)
- How filesystems work (trees, inodes, watching)
- How tools compose (decoupling via shared state)

This knowledge transfers. Git's architecture principles appear in:
- IPFS (content-addressed storage)
- Nix (functional package management)
- Blockchain systems (hash chains, immutability)
- Modern databases (immutable data structures)

Mastering git means mastering fundamental computer science concepts.