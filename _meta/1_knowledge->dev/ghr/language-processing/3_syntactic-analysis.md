# syntactic analysis


## essence

```
token stream → syntax tree
```

the parser transforms a linear sequence of tokens into a
hierarchical structure that represents the grammatical
organization of the input.

"syntax" means arrangement. the parser discovers how tokens
are arranged according to the grammar's rules.


---


## what a syntax tree is

a tree where:
- internal nodes represent grammar productions (non-terminals)
- leaf nodes represent tokens (terminals)
- parent-child relationships represent "is composed of"

```
config file:
    server {
        listen 8080;
    }

syntax tree:
    config
      └── server_block
            ├── LBRACE
            ├── server_directive
            │     ├── STRING("listen")
            │     ├── STRING("8080")
            │     └── SEMICOLON
            └── RBRACE
```

the linear sequence `server { listen 8080 ; }` becomes a tree
where "server_block contains server_directive" is explicit structure.


---


## context-free grammars

the parser's specification is a **context-free grammar** (CFG).

a CFG consists of:
- terminals: tokens (the leaves)
- non-terminals: structural categories (internal nodes)
- productions: rules of the form `A → α` where A is a
  non-terminal and α is a sequence of terminals and non-terminals
- start symbol: the top-level non-terminal

example (simplified config grammar):
```
config        → server_block { server_block }
server_block  → "server" "{" server_body "}"
server_body   → { directive }
directive     → name value { value } ";"
name          → STRING
value         → STRING
```

the grammar is the **specification**. the parser is its **recogniser**.


---


## why context-free

context-free means: the left side of every production is a single
non-terminal, without surrounding context.

```
context-free:     A → α        (A alone on left)
context-sensitive: βAγ → βαγ   (A in context of β and γ)
```

context-free grammars can express:
- nesting: server { location { } }
- recursion: expression → expression "+" term
- grouping: block → "{" statements "}"

context-free grammars cannot express:
- "variable must be declared before use"
- "function call matches function definition"
- "begin/end labels must match"

these require **context** — information from elsewhere in the tree.
they are handled by semantic analysis, not parsing.


---


## parsing algorithms

two families:

top-down: start from the start symbol, predict what to expand.
    - recursive descent (LL)
    - predictive parsing with lookahead

bottom-up: start from tokens, reduce to non-terminals.
    - LR parsing (SLR, LALR, canonical LR)
    - shift-reduce with parse tables

the ConfigFrontend uses **recursive descent**.


---


## recursive descent

each non-terminal becomes a function.
the function body mirrors the production's right-hand side.

grammar:
```
server_block → "server" "{" { directive } "}"
```

code:
```cpp
ServerConfig parse_server_block() {
    expect_STRING("server");
    expect(LBRACE);
    while (peek().type == STRING)
        parse_directive(config);
    expect(RBRACE);
    return config;
}
```

the call stack **is** the parse stack.
function calls mirror non-terminal expansions.
function returns mirror production completions.

"recursive descent" means: descend through the grammar
via recursive function calls.


---


## LL and lookahead

recursive descent implements **LL parsing**.

LL(k) means: read input Left-to-right, produce Leftmost derivation,
using k tokens of lookahead.

LL(1) is the most common: one token of lookahead.
at each decision point, the parser peeks at the next token
and decides which production to use.

this requires the grammar to be **LL(1)**: for each non-terminal,
the possible productions must be distinguishable by looking at
one token.

not LL(1):
```
statement → "if" expression "then" statement
statement → "if" expression "then" statement "else" statement
```

both start with "if" — cannot distinguish with 1 token lookahead.

solution: left-factor or increase lookahead.


---


## the parse tree vs the syntax tree

**parse tree** (concrete syntax tree): mirrors the grammar exactly.
every production creates a node. includes all tokens.

**syntax tree** (abstract syntax tree, AST): semantic structure only.
omits punctuation, collapses trivial productions.

```
parse tree:                     syntax tree:
    directive                       directive
    ├── name                        ├── "listen"
    │   └── STRING("listen")        └── "8080"
    ├── value
    │   └── STRING("8080")
    └── SEMICOLON
```

the AST discards SEMICOLON — it has no semantic content.
the AST collapses `name → STRING` — the indirection adds nothing.

in practice, most parsers build ASTs directly.
the ConfigFrontend builds `ServerConfig` structs —
an even more compact representation than a general AST.


---


## what the parser does not do

the parser does not:
- validate value ranges (is 99999 a valid port?)
- check cross-references (is this variable declared?)
- enforce semantic constraints (CGI path requires CGI extension?)

these are context-sensitive checks.
they require information from other parts of the tree.
the parser sees only local structure.


---


## error handling in parsing

when a token does not match any valid production:

fail fast:
    throw immediately with line number.
    simple, precise, but stops at first error.

error recovery:
    attempt to resynchronise and continue.
    report multiple errors in one pass.
    complex, can produce confusing cascading errors.

the ConfigFrontend uses fail-fast.
for a config file, one error is usually enough —
the operator will fix it and re-run.


---


## in other languages

Haskell (using parsec, a parser combinator library):
```haskell
serverBlock :: Parser ServerConfig
serverBlock = do
    string "server"
    braces $ many directive

directive :: Parser Directive
directive = do
    name <- identifier
    values <- many1 value
    semi
    return $ Directive name values
```

Rust (using nom, another combinator library):
```rust
fn server_block(input: &str) -> IResult<&str, ServerConfig> {
    let (input, _) = tag("server")(input)?;
    let (input, directives) = delimited(
        char('{'),
        many0(directive),
        char('}')
    )(input)?;
    Ok((input, ServerConfig { directives }))
}
```

Agda (parser indexed by grammar):
```agda
-- parse function type: proof that input matches grammar
parse : (g : CFG) → (input : List Token) 
      → Maybe (ParseTree g × List Token)
```

in dependently typed languages, the parser's type can encode
the grammar itself — a successful parse is a proof of membership.


---


## the PDA correspondence

recursive descent implements a pushdown automaton (PDA) implicitly.

the call stack is the PDA's stack.
the current function is the current state.
"call parse_directive" pushes; "return" pops.

the PDA recognises exactly the context-free languages.
the function call mechanism provides the unbounded stack
that finite automata lack.

this is why parsing requires more than lexing:
nesting requires memory proportional to nesting depth.
the call stack provides that memory.


---


## summary

syntactic analysis:
- transforms tokens to structured representation
- handles context-free structure (nesting, recursion)
- uses pushdown automata (implicit in recursive descent)
- O(n) for LL/LR grammars, O(n³) worst case
- grammar is the specification
- makes no semantic judgments beyond structure

the parser is the boundary between sequences and trees.
everything before is linear. everything after is hierarchical.