# error handling


## essence

language processing fails when input violates the language's rules.

failure must be:
- detected precisely (which rule was violated?)
- located accurately (where in the input?)
- reported clearly (what went wrong, what was expected?)


---


## error classes by phase

each phase detects a distinct class of errors.

**lexical errors**: malformed tokens
    unterminated string literal
    invalid character in source
    illegal escape sequence

**syntactic errors**: structural violations
    unexpected token (expected '{', got ';')
    premature end of input
    unmatched delimiters

**semantic errors**: meaning violations
    value out of range
    undeclared identifier
    type mismatch
    missing mandatory field
    violated coupling constraint

the phase determines what the error **is**.
the position in input determines **where** it is.


---


## error location

every error needs a location.

lexical errors: character position (line, column).
syntactic errors: token position (line from token).
semantic interpretation errors: token position (line from token).
semantic validation errors: no precise location (struct only).

the token is the carrier of location information.
once tokens become struct fields, location is lost.

this is why interpretation checks (during parsing) report
line numbers, but validation checks (after parsing) cannot.


---


## error format

a good error message contains:

1. component: [config], [http], [json] — which subsystem?
2. location: line 12, byte 847, request #3 — where?
3. expectation: expected '{', expected port number — what was wanted?
4. observation: got ';', got "abc" — what was found?

```
[config] line 12: expected port number, got "abc"
```

this format:
- identifies the subsystem
- locates the error precisely
- states what the grammar required
- states what was actually present

avoid:
```
Error: invalid input
```

this tells nothing — not where, not what, not why.


---


## error recovery strategies

**fail-fast**: stop at first error, report, exit.
    simple, precise, but only 1 error per run.

**panic mode**: skip tokens until a synchronisation point.
    e.g., skip to next `;` or `}` and resume.
    multiple errors per run, but cascading errors possible.

**phrase-level recovery**: replace malformed phrase with placeholder.
    continue parsing with the placeholder.
    complex, risk of misleading subsequent errors.

**error productions**: add grammar rules that match common errors.
    `if_stmt → IF expr THEN stmt | IF expr stmt  # missing THEN`
    explicitly anticipate mistakes.



WebServ:

ConfigFrontend
    uses fail-fast.
    rationale: config errors are fatal — the server cannot start.
    1 precise error is better than many cascading ones.

HttpRequestFrontend
    upcoming


---


## exception vs return value

2 models for propagating errors:


**exceptions**: throw at error site, catch at top level.
```cpp
// parse site
if (port < 1 || port > 65535)
    throw std::runtime_error("[config] line 12: port out of range");

// main
try { config = parser.parse(path); }
catch (const std::exception& e) { log::error(e.what()); exit(1); }
```
    + no boilerplate at each call site
    + automatic propagation through deep call chains
    − control flow is non-local, harder to trace
    − resource cleanup requires RAII or finally blocks



**return values**: return error type, propagate explicitly.
```rust
fn parse_port(s: &str, line: usize) -> Result<u16, ParseError> {
    let n: i32 = s.parse().map_err(|_| ParseError::InvalidPort(line))?;
    if n < 1 || n > 65535 { return Err(ParseError::PortRange(line)); }
    Ok(n as u16)
}
```
    + explicit control flow, visible in types
    + forces caller to handle or propagate
    − boilerplate at every call site (mitigated by `?` in Rust)
    − easy to forget to check (in languages without `Result` types)



C++ standard practice: exceptions for truly exceptional conditions.



WebServ:

ConfigFrontend:
    startup failures are fatal — exceptions appropriate.


---


## error accumulation

should the parser collect all errors or stop at first?

single error (fail-fast):
```
[config] line 12: port out of range
```

accumulated errors:
```
[config] line 12: port out of range
[config] line 15: unknown directive 'lstn'
[config] line 23: missing semicolon
```

accumulation requires:
- continuing after each error
- not letting one error corrupt subsequent state
- filtering out cascading errors (errors caused by previous errors)


WebServ:
for a config parser, fail-fast is appropriate.
the operator fixes one error, re-runs, sees the next.
the feedback loop is fast.


for a compiler processing large files, accumulation is valuable.
fix all errors in one edit/compile cycle.


---


## error taxonomy for ConfigFrontend

```
phase       error class                 location available
──────────────────────────────────────────────────────────
read        file not found              filepath
            permission denied           filepath

tokenise    (none — all bytes valid)    —

parse       unexpected token            token line
            expected X, got Y           token line
            unknown directive           token line

interpret   conversion failed           token line
            value out of range          token line

validate    mandatory field missing     server index
            coupling violated           server index
            uniqueness violated         server index
```

read errors: I/O failures.
tokenise: this lexer has no error cases (any byte sequence tokenises).
parse: structural violations.
interpret: value-level violations during parsing.
validate: cross-field violations after parsing.


---


## designing for debuggability

when an error occurs at runtime, what information aids debugging?

1. exact source location (line, column, or byte offset)
2. surrounding context (the line of input, a snippet)
3. expected vs actual
4. stack of parse context ("in server block, in location block")

example with context:
```
[config] line 12: expected port number, got "abc"
  |
  |  listen abc;
  |         ^^^
  |
```


WebServ:

ConfigFrontend
    does not include snippets — it reports line only.


for more complex languages, visual context aids comprehension.


---


## in other languages

Agda (errors as types):
```agda
data ParseResult (A : Set) : Set where
  ok    : A → ParseResult A
  error : Position → Expected → Found → ParseResult A
```

in dependently typed languages, the error structure is explicit
in the types. the type system documents what errors are possible.



Haskell (using parsec):
```haskell
-- parsec provides detailed error with expected/unexpected
-- and stack of labels
parse serverBlock "config" input
-- on failure:
-- "config" (line 12, column 8):
-- unexpected ";"
-- expecting "{"
```



Rust (using nom with custom errors):
```rust
fn parse_port(input: &str) -> IResult<&str, u16, CustomError> {
    let (input, digits) = digit1(input)?;
    let port: u16 = digits.parse()
        .map_err(|_| nom::Err::Failure(CustomError::InvalidPort))?;
    Ok((input, port))
}
```


---


## information to add to doc

Ontological gaps:

Partiality as foundation - errors ARE partiality.
Parser: String → Maybe Tree.
Lexer: String → Maybe [Token].
Error handling is making partiality explicit in types and control flow. This grounds everything.

Error as value vs error as effect - the fundamental dichotomy.
Either E A (error is data, first-class) vs throw/catch (error is control flow, effect).
Different ontological status. Different composition laws.

Algebra of failure - how partial computations compose. Monad laws.
Short-circuit semantics. e1 >>= e2 fails if either fails.
Applicative vs monadic error accumulation.

Error provenance / causality - errors cause errors. Causal chains.
"Caused by" relation. Currently no structure for multi-level failure.

Recoverability criteria - document lists strategies but not the decision function.
What makes an error recoverable? Domain knowledge encoded how?


Mechanical gaps:

Typed error hierarchies - sum types reflecting failure modes.
LexError | ParseError | SemanticError. Exhaustiveness checking.

Context accumulation - Writer-like threading of diagnostic context.
How stack traces work. Source spans carried through transformations.



Re: more general term than "error-handling":

Partiality - the upstream mathematical concept.
A partial function may not yield a value for all inputs.
Error handling is the discipline of making partiality explicit and manageable.

Failure semantics - PL theory level. What does failure mean?
What information accompanies it? How does it propagate?

"Error handling" is downstream, implementation-flavoured. "Partiality" is the ontology.
