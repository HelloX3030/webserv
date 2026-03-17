## lexical analysis: hand-written scanner

### the question

should the lexer be a table-driven DFA (compiled from regex specifications)
or a hand-written character loop?

### the analysis

table-driven DFAs earn their complexity when:
- multiple token types compete at the same position (maximal munch)
- keywords vs identifiers require priority resolution
- patterns have non-trivial structure (nested quotes, escape sequences)

HTTP/1.1 request lexical structure:
```
METHOD       ::= "GET" | "POST" | "DELETE" | ...
SP           ::= ' '
URI          ::= (non-whitespace)+
CRLF         ::= '\r' '\n'
HEADER_NAME  ::= (non-colon, non-whitespace)+
HEADER_VALUE ::= (non-CRLF)*
```

no ambiguity. no competing interpretations. tokens are positionally determined:
after METHOD comes SP, after SP comes URI, etc. the "lexer" is not choosing
between token types — it is reading a known sequence.

contrast with a programming language where `if` could be keyword or
identifier prefix, where `3.14` could be float or `3` followed by `.14`.
those require the DFA machinery. HTTP does not.

### the decision

hand-written scanner. a table-driven DFA would obscure trivial logic
behind generated tables, adding complexity without solving a real problem.

### the principle

use the weakest sufficient mechanism.
DFA: O(n) with constant factors from table lookup.
hand-written: O(n) with lower constants, transparent logic.
when both achieve the same complexity class and the input is simple,
choose the one that reveals intent.
