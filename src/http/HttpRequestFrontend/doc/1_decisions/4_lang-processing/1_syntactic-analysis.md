## syntactic analysis: no LR parsing

### question

should the parser use bottom-up (LR) techniques — shift-reduce with
parse tables — or top-down recursive descent?

### the analysis

LR parsing is designed for grammars with:
- deep nesting: `{ { { ... } } }`
- recursive productions: `expr → expr + term`
- ambiguity requiring precedence/associativity resolution

HTTP request structure:
```
request      = request-line headers CRLF body?
request-line = method SP uri SP version CRLF
headers      = (header-line)*
header-line  = name ":" value CRLF
body         = <Content-Length bytes>
```

no nesting. no recursion. the only repetition is `headers*`, which is
Kleene star — a regular (type 3) construct, not requiring a stack.

the Chomsky hierarchy classifies this:
- type 3 (regular): finite automaton suffices
- type 2 (context-free): pushdown automaton (stack) required
- type 1 (context-sensitive): bounded Turing machine

HTTP request syntax is type 3. the one context-sensitive aspect —
Content-Length determining body size — is semantic, handled after
header parsing completes.

LR machinery (handle recognition, viable prefix computation, parse tables)
solves problems HTTP does not have.

### decision

recursive descent. or more precisely: a state machine that is simpler
than recursive descent because there is no recursion. phase transitions
(REQUEST_LINE → HEADERS → BODY → COMPLETE) with no call stack depth.

### the principle

match formalism to problem structure.
LR parsing: context-free grammars with nesting.
recursive descent: context-free grammars, natural for LL(1).
state machine: regular structure with phases.
HTTP is the third case.
