# manifestations


## the archetype

```
linear input → lexical → syntactic → semantic → verified structure
```

this pattern recurs wherever text must become structure.
the domain changes. the form persists.


---


## compilers

the original domain. source code → executable.

```
source code
    │
    ├─ lexer         → tokens (identifiers, operators, literals)
    ├─ parser        → AST (abstract syntax tree)
    ├─ semantic      → typed AST (type-checked, resolved)
    ├─ optimiser     → transformed AST/IR
    └─ codegen       → machine code / bytecode
```

examples: GCC, Clang, GHC, rustc, Agda.

the frontend (lex → parse → semantic) is the language processing.
the backend (optimise → codegen) is language-independent.


---


## interpreters

source code → immediate execution.

```
source code
    │
    ├─ lexer         → tokens
    ├─ parser        → AST
    ├─ semantic      → validated AST
    └─ evaluator     → result
```

examples: Python, Ruby, Lisp, shell scripts.

same frontend as compilers.
instead of code generation: tree-walking evaluation.


---


## protocol parsers

wire format → structured messages.

```
bytes on wire
    │
    ├─ framing       → delimited messages
    ├─ lexer         → tokens (headers, body sections)
    ├─ parser        → request/response structure
    └─ validator     → semantic checks (content-length matches body?)
```

examples: HTTP, SMTP, DNS, FTP, TLS handshake.

HTTP request parsing:
```
GET /index.html HTTP/1.1\r\n
Host: example.com\r\n
\r\n
body...

    ↓ tokenise (split by \r\n, split headers by :)

method: GET, path: /index.html, version: HTTP/1.1
headers: [(Host, example.com)]
body: ...

    ↓ validate (is method known? is path valid? does content-length match?)

HttpRequest { method, path, version, headers, body }
```


---


## configuration parsers

config file → runtime parameters.

```
config file
    │
    ├─ reader        → raw string
    ├─ lexer         → tokens (braces, semicolons, strings)
    ├─ parser        → config tree (server blocks, directives)
    └─ validator     → semantic checks (mandatory fields, couplings)
```

examples: NGINX, Apache, systemd, SSH config.

the ConfigFrontend is this archetype instantiated for webserv.


---


## data format parsers

serialised data → in-memory structures.

```
serialised bytes
    │
    ├─ lexer         → tokens (brackets, colons, strings, numbers)
    ├─ parser        → tree structure
    └─ (validator)   → (schema validation, if schema exists)
```

examples: JSON, XML, YAML, TOML, Protocol Buffers, MessagePack.

JSON parsing:
```
{"name": "alice", "age": 30}

    ↓ tokenise

LBRACE STRING("name") COLON STRING("alice") COMMA ...

    ↓ parse

JsonObject { fields: [("name", JsonString("alice")), ("age", JsonNumber(30))] }
```

no semantic phase unless a schema is provided.
without schema, any valid JSON tree is acceptable.


---


## query languages

query text → execution plan.

```
query string
    │
    ├─ lexer         → tokens (keywords, identifiers, operators)
    ├─ parser        → AST (select, from, where, joins)
    ├─ semantic      → resolved AST (table names resolved, types checked)
    ├─ planner       → execution plan
    └─ executor      → result set
```

examples: SQL, GraphQL, XPath, XQuery, SPARQL.

SQL parsing:
```
SELECT name, age FROM users WHERE age > 30

    ↓ parse

Select {
    columns: [Column("name"), Column("age")],
    from: Table("users"),
    where: BinaryOp(Column("age"), >, Literal(30))
}

    ↓ semantic (resolve table, check column existence, check types)

ResolvedSelect { ... }
```

the semantic phase resolves names against the database schema.


---


## markup processors

marked-up text → rendered output.

```
marked-up source
    │
    ├─ lexer         → tokens (commands, text, environments)
    ├─ parser        → document tree
    ├─ semantic      → resolved tree (cross-references, bibliography)
    └─ renderer      → output format (PDF, HTML)
```

examples: LaTeX, Markdown, HTML, DocBook, reStructuredText.

LaTeX processing:
```
\section{Introduction}
This is text with a \cite{ref2024}.

    ↓ parse

Document [
    Section { title: "Introduction", content: [
        Text("This is text with a "),
        Citation("ref2024"),
        Text(".")
    ]}
]

    ↓ semantic (resolve citations against bibliography)

Document [ Section { ... Citation { key: "ref2024", resolved: BibEntry } } ]
```


---


## domain-specific languages (DSLs)

specialised notation → domain operations.

```
DSL source
    │
    ├─ lexer         → tokens
    ├─ parser        → AST
    ├─ semantic      → validated, resolved AST
    └─ (interpreter or compiler)
```

examples: regular expressions, CSS selectors, cron expressions,
Makefile rules, Terraform configurations, Dockerfile commands.

regex compilation:
```
[a-z]+@[a-z]+\.[a-z]{2,4}

    ↓ parse

Concat [
    OneOrMore(CharClass('a'..'z')),
    Literal('@'),
    OneOrMore(CharClass('a'..'z')),
    Literal('.'),
    Repeat(CharClass('a'..'z'), 2, 4)
]

    ↓ compile to NFA/DFA

(state machine)
```


---


## the invariants

across all manifestations:

1. **lexical phase** handles regular structure.
   finite patterns, no nesting, O(n).

2. **syntactic phase** handles context-free structure.
   nesting, recursion, tree building.

3. **semantic phase** handles context-sensitive constraints.
   cross-references, type checking, validation.

4. **errors** are classified by phase and located by source position.

5. the **grammar** (or protocol spec, or schema) is the specification.
   the code is its recogniser.


---


## why the pattern persists

it emerges from the structure of the problem.

text is linear. programs need structure.
bridging linearity to structure requires hierarchy of formalisms.
the Chomsky hierarchy provides that hierarchy.
the pipeline phases implement that hierarchy efficiently.

any system that processes structured text will rediscover this pattern.
understanding it at the archetypal level means:
- recognising it in any new domain
- knowing where to look for each class of error
- knowing what formal model applies at each stage