# language processing — overview


## essence

the fundamental problem:

```
linear sequence of characters → structured representation
```

characters have no inherent structure. a program that must act on
textual input cannot reason about characters — it needs typed values,
hierarchical relationships, semantic constraints.

the transformation from linearity to structure is **language processing**.


---


## why "language"

the input is not arbitrary characters. it conforms to rules.
those rules constitute a **language** in the formal sense:
a set of strings over an alphabet, defined by a grammar.

the transformation succeeds iff the input belongs to the language.
failure means the input is not a valid sentence in that language.

this framing — input as language membership — connects the practical
problem (parse this config file) to the mathematical foundation
(formal language theory, automata theory, type theory).


---


## the universal pipeline

every language processor follows the same archetypal structure:

```
characters
    │
    │  lexical analysis
    v
tokens
    │
    │  syntactic analysis
    v
syntax tree
    │
    │  semantic analysis
    v
verified structure
    │
    │  (optional: execution / code generation)
    v
result
```

each phase has:
- a distinct input type
- a distinct output type
- a distinct class of errors it can detect
- a distinct formal model that describes it

the phases are not arbitrary engineering choices.
they correspond to levels in the Chomsky hierarchy —
fundamentally different classes of computational problems.


---


## where this exists

the pipeline manifests wherever text becomes structure:

compilers and interpreters:
    source code → executable / evaluated result
    (GCC, Clang, GHC, Agda, rustc)

protocol parsers:
    wire format → request/response structures
    (HTTP, SMTP, DNS, TLS handshake)

configuration parsers:
    config file → runtime parameters
    (NGINX, systemd, TOML, YAML, JSON)

data format parsers:
    serialised data → in-memory structures
    (JSON, XML, Protocol Buffers, ASN.1)

query languages:
    query text → execution plan
    (SQL, GraphQL, XPath)

markup processors:
    marked-up text → rendered output
    (LaTeX, Markdown, HTML)

the domain changes. the structure persists.


---


## telos

understand the pipeline at the archetypal level:
- why each phase exists (logical necessity, not convention)
- what formal model governs each phase
- what class of errors each phase detects
- how the phases compose

this understanding transfers across every instantiation.
a config parser and an HTTP parser and a compiler frontend
are the same archetype with different grammars.


---


## document structure

```
0_overview.md           — this file. essence, scope, telos.
1_formal-foundations.md — Chomsky hierarchy, automata correspondence.
2_lexical-analysis.md   — characters → tokens. regular languages. DFA.
3_syntactic-analysis.md — tokens → tree. context-free grammars. parsing algorithms.
4_semantic-analysis.md  — tree → verified structure. type checking. validation.
5_error-handling.md     — error classes, recovery, reporting.
6_manifestations.md     — how the archetype appears in specific domains.
```