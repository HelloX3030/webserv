# semantic analysis


## essence

```
syntax tree → verified, typed structure
```

semantic analysis transforms the syntactically valid structure
into a semantically valid structure.


---


## what syntax cannot express

context-free grammars cannot express:

"variable x must be declared before use":
    requires tracking declarations across the tree.
    
"function call arguments must match parameter types":
    requires comparing distant tree nodes.

"CGI extension and CGI path must both be present or both absent":
    requires checking relationships between sibling fields.

"port number must be in range [1, 65535]":
    requires interpreting the value, not just its form.

these are **context-sensitive** constraints.
they depend on information not available at the local parse site.


---


## two sub-phases

semantic analysis divides into two distinct operations:

**interpretation**: token value → typed value
    STRING("8080") → uint16_t(8080)
    STRING("10m")  → size_t(10485760)
    STRING("on")   → bool(true)

**validation**: complete structure → verified structure
    all mandatory fields present?
    all cross-references resolved?
    all semantic couplings satisfied?

these are separated by **when** they can run and **what context**
they have access to.


---


## interpretation

interpretation converts syntactic values to semantic values.

```
syntactic:  Token { type: STRING, value: "8080", line: 12 }
semantic:   uint16_t(8080)
```

the parser saw a STRING token.
interpretation reveals it represents a port number.

interpretation operates **during parsing**, at the point where
a value is consumed. it has access to:
- the token's string value
- the token's line number (for error messages)
- the type expected by the grammar position

interpretation checks:
- is the string convertible to the target type?
- is the converted value within the type's semantic domain?

```cpp
uint16_t parse_port(const std::string& s, size_t line) {
    int n = std::stoi(s);          // convertible?
    if (n < 1 || n > 65535)        // in domain?
        throw ...;
    return static_cast<uint16_t>(n);
}
```

the range check [1, 65535] is not type-level — `uint16_t` admits 0.
it is domain-level — port 0 is not a valid service binding.

interpretation is **local**: one token, one value, one check.
it produces typed values that populate the structure.


---


## validation

validation checks the complete structure for semantic coherence.

```
input:  std::vector<ServerConfig>  (all fields populated)
output: std::vector<ServerConfig>  (verified)
```

validation operates **after parsing**, on the assembled structure.
it has access to:
- all field values
- relationships between fields
- the entire configuration

validation checks:
- mandatory field presence
- cross-field couplings
- uniqueness constraints
- completeness invariants

```cpp
void validate_location(const Location& loc) {
    if (loc.root.empty())
        throw ...;  // mandatory field

    if (loc.cgi_extension.has_value() != loc.cgi_path.has_value())
        throw ...;  // coupling: both or neither

    if (loc.upload_enable && loc.upload_store.empty())
        throw ...;  // coupling: enable implies store
}
```

validation is **global**: the whole structure, all relationships.


---


## why the separation

interpretation must happen during parsing because:
- line numbers are available (for error messages)
- the transformation produces values that populate the struct
- deferring would require storing raw strings everywhere

validation must happen after parsing because:
- mandatory field checks require the complete struct
- coupling checks require multiple fields
- the parser builds incrementally — completeness is only known at end

attempting to merge them fails:
- interpret at validate-time: no line numbers, errors are imprecise
- validate at parse-time: struct incomplete, checks are premature

the separation is not a design choice. it is a logical necessity
arising from the information available at each stage.


---


## redundant checking

some constraints are checked at both stages:

```cpp
// interpretation (parse time): with line number
if (port < 1 || port > 65535)
    throw "[config] line 12: port out of range";

// validation (post-parse): over struct
for (const auto& addr : server.listen)
    if (addr.port < 1 || addr.port > 65535)
        throw "[config] validation error: port out of range";
```

why both?

interpretation catches the error at the source, with precise location.
validation catches any code path that might bypass interpretation.

this is **defense in depth**. the validation check is a safety net.
if both pass, the invariant holds. if either fails, the error
is caught before runtime.


---


## type checking as semantic analysis

in programming languages, type checking is the central semantic task.

```
let x = 5
let y = "hello"
let z = x + y      // type error: cannot add int and string
```

the parser accepts `x + y` — it is syntactically valid.
the type checker rejects it — the operands are type-incompatible.

type checking requires:
- symbol table: mapping names to types
- type environment: scoped type bindings
- type inference/checking algorithm

the ConfigFrontend has no type system in this sense.
its "types" are fixed by the grammar position:
"after 'listen' comes a host:port" — not inferred, specified.


---


## symbol tables and environments

for languages with names (variables, functions), semantic analysis
builds auxiliary structures:

**symbol table**: maps names to declarations
    "x" → { type: int, scope: function foo, line: 12 }

**type environment**: maps names to types in scope
    { x: int, y: string, foo: int → int }

these structures enable:
- name resolution: "which 'x' does this refer to?"
- scope checking: "is 'x' visible here?"
- type checking: "what is the type of 'x + y'?"

the ConfigFrontend has no symbolic names in this sense.
directive names are fixed keywords, not user-defined symbols.


---


## error messages

interpretation errors have line numbers:
```
[config] line 12: port out of range — value: 99999, valid: [1, 65535]
```

validation errors do not:
```
[config] validation error: server block has no listen directive
```

once the struct is built, source locations are lost.
the token carried line numbers; the struct field does not.

this is a trade-off:
- carrying source locations in every field: memory overhead, complexity
- losing them: less precise validation errors

for a config file, the trade-off is acceptable.
for a programming language compiler, source locations are typically
preserved in the AST for precise diagnostics.


---


## in other languages

Haskell (type checking via unification):
```haskell
typeCheck :: Expr → TypeEnv → Either TypeError Type
typeCheck (Add e1 e2) env = do
    t1 ← typeCheck e1 env
    t2 ← typeCheck e2 env
    unify t1 TInt
    unify t2 TInt
    return TInt
```

Rust (borrow checker as semantic analysis):
```rust
// semantic constraint: cannot use moved value
let s1 = String::from("hello");
let s2 = s1;
println!("{}", s1);  // error: value moved
```

Rust's borrow checker is semantic analysis — it checks constraints
(ownership, lifetimes) that cannot be expressed in the grammar.

Agda (types are proofs):
```agda
-- semantic constraint encoded in type
data ValidPort : ℕ → Set where
  valid : (n : ℕ) → 1 ≤ n → n ≤ 65535 → ValidPort n

-- port that cannot be invalid
parsePort : String → Maybe (Σ ℕ ValidPort)
```

in Agda, the type itself carries the constraint.
a `ValidPort` cannot exist unless the proof obligations are met.
semantic analysis becomes type checking.


---


## summary

semantic analysis:
- transforms syntax tree to verified structure
- handles context-sensitive constraints
- no uniform automaton — ad hoc algorithms
- two sub-phases: interpretation and validation
- interpretation: local, during parsing, has line numbers
- validation: global, after parsing, checks relationships