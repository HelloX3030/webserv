# formal foundations


## the Chomsky hierarchy

Noam Chomsky (1956) classified formal languages into 4 types
based on the generative power of their grammars.

```
type 0: recursively enumerable    (unrestricted grammars)
type 1: context-sensitive         (αAβ → αγβ, |γ| ≥ 1)
type 2: context-free              (A → γ)
type 3: regular                   (A → aB or A → a)
```

these form a strict inclusion chain:

```
type 3 ⊂ type 2 ⊂ type 1 ⊂ type 0
```

every regular language (type 3) is also context-free (type 2).
every context-free language is also context-sensitive (type 1).
and so on.

lower type number means greater generative power.
higher type number means more restrictive grammar rules.

the hierarchy matters because each type corresponds to
a different computational model with different power and cost.


---


## automata correspondence

each language type has a minimal automaton that recognises it:

```
language type           recognising automaton
─────────────────────────────────────────────────────
type 3 (regular)        finite automaton (DFA/NFA)
type 2 (context-free)   pushdown automaton (PDA)
type 1 (context-sens.)  linear-bounded automaton (LBA)
type 0 (rec. enum.)     Turing machine (TM)
```

the automaton defines what computational resources are needed
to decide membership in that language type.

type 3 — regular languages:
    finite memory suffices.
    the automaton has fixed states, no auxiliary storage.
    recognition is O(n) in input length.

type 2 — context-free languages:
    a stack suffices.
    the automaton has states plus unbounded stack.
    recognition is O(n³) in general, O(n) for deterministic subclass.

type 1 — context-sensitive languages:
    tape bounded by input length.
    recognition is PSPACE-complete in general.

type 0 — recursively enumerable:
    unbounded tape.
    recognition is undecidable in general (halting problem).


---


## pipeline phases mapped to the hierarchy

the language processing pipeline corresponds to hierarchy levels:

```
phase                   language type       automaton       typical complexity
──────────────────────────────────────────────────────────────────────────────
lexical analysis        type 3 (regular)    DFA             O(n)
syntactic analysis      type 2 (CF)         PDA             O(n) to O(n³)
semantic analysis       type 1 (CS)         (ad hoc)        varies
```

lexical analysis handles type 3 (regular) structure:
    identifiers, numbers, string literals, keywords.
    no nesting, no recursion, no unbounded matching.

syntactic analysis handles type 2 (context-free) structure:
    nested blocks, matched parentheses, recursive definitions.
    requires memory proportional to nesting depth.
    a stack suffices — hence pushdown automata.

semantic analysis handles type 1 (context-sensitive) constraints:
    "variable must be declared before use"
    "function call arguments must match parameter types"
    "CGI extension requires CGI path"
    these depend on context not expressible in context-free grammar.
    no single automaton — handled by ad hoc algorithms
    over the syntax tree (symbol tables, type environments).


---


## why the separation is necessary

could a single phase handle everything?

theoretically: yes. a Turing machine can recognise any
recursively enumerable language, which includes all programs.

practically: no. the cost is prohibitive.

type 3 recognition: O(n), constant space.
type 2 recognition: O(n) for LL/LR grammars, O(n³) general.
type 1 recognition: PSPACE-complete.

separating phases lets each use the minimal machinery:
- lexer: DFA, O(n), table-driven, extremely fast
- parser: PDA, O(n) for practical grammars, recursive descent
- semantic: tree walks, symbol tables, type inference

the separation is not modularity for its own sake.
it is **computational economy** — using the weakest sufficient model.


---


## the type 3 / type 2 boundary

the lexer/parser split occurs at the regular/context-free boundary.

regular languages (type 3) cannot express:

matched parentheses:
    the language { (ⁿ)ⁿ | n ≥ 0 } — n opens followed by n closes.
    "()", "(())", "((()))" are in the language.
    "(", "())", "(()" are not.
    a finite automaton cannot verify that opens equal closes.

nested structures:
    arbitrarily deep nesting like { { { } } }.
    the automaton would need to count depth, but has no counter.

cross-serial dependencies:
    the language { aⁿbⁿ | n ≥ 0 } — n copies of 'a' followed by n copies of 'b'.
    "ab", "aabb", "aaabbb" are in the language.
    "aab", "abb" are not.

the proof: regular languages are recognised by finite automata.
finite automata have fixed, finite states.
counting to arbitrary n requires unbounded memory.
therefore: regular languages cannot count unboundedly.

context-free grammars can express these:

```
matched_parens → "(" matched_parens ")" | ε
aⁿbⁿ          → "a" aⁿbⁿ "b" | ε
```

the grammar's recursion, implemented via a stack, provides
the unbounded memory for matching.

this is why lexers handle tokens (finite patterns)
and parsers handle structure (nested patterns).


---


## the type 2 / type 1 boundary

context-free grammars (type 2) cannot express:

three-way matching:
    the language { aⁿbⁿcⁿ | n ≥ 0 }.
    "abc", "aabbcc", "aaabbbccc" are in the language.
    a stack can match two counts but not three simultaneously.

declaration-before-use:
    "identifier x must be declared before any use of x."
    requires remembering all declarations while parsing uses.

type agreement:
    "function call arguments must match parameter types."
    requires comparing distant parts of the parse tree.

these require checking relationships across the structure —
**context** in the linguistic sense.

semantic analysis handles these constraints.
it operates on the completed syntax tree, with auxiliary structures
(symbol tables, type environments) that track context.

no grammar formalism is used at this stage.
the algorithms are designed for the specific constraints of the language.


---


## implications for implementation

understanding the hierarchy guides implementation choices:

lexer design:
    regular expressions suffice for token specification.
    compile to DFA for O(n) recognition.
    no recursion needed, no stack, no lookahead beyond fixed k.

parser design:
    context-free grammar is the specification.
    recursive descent is natural for LL grammars.
    each grammar production becomes a function.
    function call stack *is* the PDA stack.

semantic analysis design:
    no grammar formalism — constraint checking over the tree.
    symbol tables for name resolution.
    type environments for type checking.
    validation functions for domain constraints.


---


## references

Chomsky, N. (1956). "Three models for the description of language."
    IRE Transactions on Information Theory, 2(3), 113–124.
    the original hierarchy paper.

Hopcroft, J., Motwani, R., Ullman, J. (2006).
    Introduction to Automata Theory, Languages, and Computation.
    standard textbook on formal languages and automata.

Aho, A., Lam, M., Sethi, R., Ullman, J. (2006).
    Compilers: Principles, Techniques, and Tools. ("The Dragon Book")
    standard textbook on compiler construction.

Sipser, M. (2012).
    Introduction to the Theory of Computation.
    rigorous treatment of computability and complexity.