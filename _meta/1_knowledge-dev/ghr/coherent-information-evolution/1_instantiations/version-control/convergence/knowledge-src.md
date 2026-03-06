# Convergence / Version Control: Knowledge Sources

to generalise to general domain of 
version-control, coherence...

## Tier 0: Normative/Specification

### Git Internals Documentation

**Source:** https://git-scm.com/docs/gitformat-pack
         https://git-scm.com/docs/gitformat-index
         https://git-scm.com/docs/gitformat-commit-graph

**What it is:** Technical specification of Git's file formats and protocols.

**Sections:**
- Pack format (object storage)
- Index format (staging area)
- Commit-graph format (DAG acceleration structure)
- Protocol documentation (fetch/push wire protocol)

**Use:** Final arbiter of "what Git actually does" at byte level.

---

### Git Source Code

**Source:** https://github.com/git/git

**Language:** C

**Key files for convergence:**
- `merge-ort.c` — current merge algorithm ("Ostensibly Recursive's Twin")
- `merge-recursive.c` — older recursive merge strategy
- `diff.c` — diff computation
- `revision.c` — DAG traversal, ancestry
- `refs.c` — reference management

**Use:** Ground truth. When documentation unclear, code decides.

---

## Tier 1: Foundational/Authoritative

### Darcs: Patch Theory

**Papers:**

David Roundy. *A Distributed Version Control System*. (2005)
https://darcs.net/Theory

Judah Jacobson. *A Formalization of Darcs Patch Theory Using Inverse Semigroups*. (2009)
https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=f6e7e0e0f9e4e8c8e7e8e9e0e7e8e9e7e8e9e0e7

**Why essential:** Darcs treats patches as first-class algebraic objects with composition, inversion, commutation. This is the most mathematically rigorous approach to version control theory.

**Key concepts:**
- Patch as morphism between repository states
- Commutation: when can patches be reordered?
- Merge as pushout (category-theoretic framing)
- Inverse patches (every patch has an undo)

**Relevance:** Git's merge is ad-hoc compared to Darcs' principled theory. Understanding Darcs illuminates what Git *approximates*.

---

### Pijul: Category-Theoretic Version Control

**Paper:**

Pierre-Étienne Meunier. *Pijul: Categorical Version Control*. 
https://pijul.org/manual/theory.html

**Why essential:** Pijul formalizes version control using category theory explicitly. Patches form a category; merge is a pushout.

**Key concepts:**
- Repository states as objects
- Patches as morphisms
- Pushout = merge (universal property)
- Handles "graggles" (generalized graphs) for conflict representation

**Relevance:** The upstream mathematical reality that Git's merge instantiates imperfectly.

---

### Three-Way Merge: Original Paper

Sanjeev Khanna, Keshav Kunal, Benjamin C. Pierce. *A Formal Investigation of Diff3*. (2007)
https://www.cis.upenn.edu/~bcpierce/papers/diff3-short.pdf

**Why essential:** Formal analysis of the diff3/three-way merge algorithm. Proves properties, identifies failure modes.

**Key concepts:**
- Definition of "merge" as function
- Conditions for conflict-free merge
- Relationship between diff and merge

---

### Operational Transformation (OT)

Clarence Ellis, Simon Gibbs. *Concurrency Control in Groupware Systems*. (1989)
ACM SIGMOD Record 18(2): 399-407

Chengzheng Sun, Clarence Ellis. *Operational Transformation in Real-Time Group Editors: Issues, Algorithms, and Achievements*. (1998)
Proceedings of CSCW '98

**Why essential:** OT is the theoretical foundation for real-time collaborative editing (Google Docs, etc.). Different approach to the same problem: reconciling concurrent modifications.

**Key concepts:**
- Transformation functions
- Convergence property
- Intention preservation
- Comparison with patch-based approaches

---

### CRDTs: Conflict-Free Replicated Data Types

Marc Shapiro, Nuno Preguiça, Carlos Baquero, Marek Zawirski. *A Comprehensive Study of Convergent and Commutative Replicated Data Types*. (2011)
INRIA Technical Report RR-7506
https://hal.inria.fr/inria-00555588/document

**Why essential:** CRDTs guarantee eventual consistency without coordination. Alternative mathematical framework to patch theory.

**Key concepts:**
- State-based vs operation-based CRDTs
- Lattice structure (join-semilattice)
- Monotonic merge functions
- Strong eventual consistency

**Relevance:** Modern distributed systems (IPFS, local-first software) use CRDTs. Understanding this illuminates limitations of Git's model.

---

## Tier 2: Pedagogical/Authoritative Intermediate

### Pro Git (Scott Chacon, Ben Straub)

**Source:** https://git-scm.com/book/en/v2

**Key chapters:**
- Chapter 10: Git Internals — object model, refs, packfiles
- Chapter 3.2: Basic Branching and Merging
- Chapter 3.6: Rebasing

**Use:** Best intermediate explanation of Git's actual mechanics. Written by GitHub co-founder.

---

### Git from the Bottom Up (John Wiegley)

**Source:** https://jwiegley.github.io/git-from-the-bottom-up/

**What it is:** Explains Git by starting from content-addressable storage and building up.

**Use:** For understanding Git's object model before strategies.

---

## Tier 3: Reference Implementations

### Git itself

See Tier 0.

### Jujutsu (jj)

**Source:** https://github.com/martinvonz/jj

**Creator:** Martin von Zweigbergk (Google)

**Why study:** Modern rethinking of Git's UI while keeping content-addressable store. Different approach to same DAG operations.

**Key innovations:**
- First-class conflicts (conflicts are committable)
- Operation log (undo anything)
- Anonymous branches by default

---

### Pijul

**Source:** https://pijul.org/

**Why study:** Production implementation of category-theoretic patch model. See Tier 1 theory in working code.

---

### Darcs

**Source:** https://darcs.net/

**Language:** Haskell

**Why study:** Original patch-theoretic VCS. Haskell implementation means closer correspondence between theory and code.

---

## Tier 4: Instrumental/Tools

```
git merge-base        # find LCA
git merge-tree        # preview merge without committing
git diff              # examine deltas
git reflog            # recover "lost" commits
git rev-list          # traverse DAG
git cat-file          # inspect objects directly
git fsck              # verify object store integrity
```

---

## Tier 5: Philosophical/Architectural

### Linus Torvalds on Git Design

**Talk:** *Tech Talk: Linus Torvalds on Git* (2007)
https://www.youtube.com/watch?v=4XpnKHJAok8

**Why essential:** Design rationale from creator. Why content-addressable? Why DAG? Why distributed?

---

### Git Merge Strategies: Design Rationale

**Source:** https://git-scm.com/docs/merge-strategies

Junio C Hamano (Git maintainer) posts on git mailing list.

**Why study:** Explains *why* multiple strategies exist, tradeoffs each makes.

---

### Version Control by Example (Eric Sink)

**Source:** https://ericsink.com/vcbe/

**What it is:** Comparative analysis across VCS paradigms (centralized, distributed, lock-based, merge-based).

**Use:** Understand Git's design choices against alternatives.

---

## Upstream Theory (Mathematics)

### Category Theory for Programmers (Bartosz Milewski)

**Source:** https://bartoszmilewski.com/2014/10/28/category-theory-for-programmers-the-preface/

**Relevant chapters:**
- Limits and Colimits
- Pushouts and Pullbacks

**Use:** Understand pushout as universal construction. Merge *is* pushout.

---

### Lattice Theory / Order Theory

Davey & Priestley. *Introduction to Lattices and Order*. Cambridge University Press.

**Relevance:** CRDTs rely on join-semilattice structure. DAG itself is a partial order. Understanding order theory → understanding version control at mathematical level.

---

### Groupoids and Inverse Semigroups

Mark V. Lawson. *Inverse Semigroups: The Theory of Partial Symmetries*. World Scientific.

**Relevance:** Patch theory (Darcs) can be formalized using inverse semigroups. Deep mathematical foundation.

---

## Reading Order (Suggested)

1. **Pro Git Ch. 10** — establish Git's object model
2. **Git from the Bottom Up** — reinforce object model
3. **diff3 paper** — understand three-way merge formally
4. **Darcs theory** — see patches as algebraic objects
5. **Pijul theory** — category-theoretic framing
6. **CRDT paper** — alternative consistency model
7. **Category Theory for Programmers** (limits/colimits) — understand pushout
8. **Git source code** — see it implemented

---

## Anti-Sources (Avoid)

- Medium/Dev.to "git rebase explained" posts — usually wrong or superficial
- Stack Overflow answers on merge vs rebase — opinion wars, no theory
- "Git workflow" corporate blogs — cargo cult, no understanding
- YouTube "tutorials" — time-inefficient, rarely rigorous

Exception: Torvalds' original talk (Tier 5) is worth the video format.