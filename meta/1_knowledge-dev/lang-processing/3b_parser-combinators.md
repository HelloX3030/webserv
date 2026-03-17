# parser combinators

upcoming.


---


## scope

- combinators as algebra: the mathematical structure underlying composition
- parsers as values: `Parser a` as first-class, composable objects
- the monadic interface: `pure`, `>>=`, `<|>`, `many`, `some`
- applicative vs monadic: when sequence suffices, when binding is necessary
- error handling in combinators: `try`, `<?>`, labels, backtracking
- relationship to recursive descent: combinators as abstracted recursive descent
- implementations: parsec (Haskell), nom (Rust), attoparsec (Haskell, streaming)
- when to use: readability vs performance trade-offs
- connection to formal grammars: combinator structure mirrors BNF


---


## placement rationale

sibling to syntactic analysis (3_).
parser combinators are an alternative *implementation strategy* for
context-free parsing — same Chomsky level, different realisation.
not a separate phase; a different way to build the parser phase.
