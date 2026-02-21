# parse phase — design

---

## what the parser does

the parser receives a flat token sequence produced by the tokeniser.
it must produce a structured representation of the config: a
`std::vector<ServerConfig>`.

the transformation: a linear sequence of typed tokens → a tree of
typed, semantically meaningful structs.

the token sequence has no inherent structure. the parser imposes
structure by reading tokens in an order defined by the grammar,
building structs as it goes, throwing on any deviation.

---

## recursive descent — what the term means

each grammar production rule becomes a function.
the call hierarchy of those functions mirrors the nesting hierarchy
of the grammar.

```
grammar:
config → server_block → { server_dir | location_block }
                             location_block → { location_dir }

call tree:
parse_config
  └─ parse_server_block
       ├─ parse_server          (for flat directives)
       │    └─ parse_listen / parse_server_name / ...
       └─ parse_location_block  (for location {} blocks)
            └─ parse_location
                 └─ parse_root / parse_methods / ...
```

"descent" means: the parser begins at the top-level production
(`parse_config`) and descends toward terminal tokens.
"recursive" describes the structural mirroring — not self-calls.
self-calls would appear only if the grammar were self-referential
(e.g. a location block permitted nested location blocks).
this grammar has no such nesting, so no function calls itself.

---

## file structure — ordering convention

5 translation units, lettered by dependency:

```
3a_navigate   — primitives. every other file calls these.
3b_blocks     — grammar productions: config, server, location blocks.
3c_server     — server-level directive parsers.
3d_location   — location-level directive parsers.
3e_interpret  — leaves: host_port, port, size.
```

lettering reflects dependency order, not grammar level.
`3a` comes first because `3b`–`3e` all call `peek`, `consume`,
`expect*`. reading a file before its dependencies creates undefined
references.

alternative ordering by grammar level (config → server → location
→ leaves) is coherent but inverts the dependency direction —
the reader encounters call sites before definitions.

---

## navigation helpers

### peek and consume — the asymmetry

`peek()` is `const`: it reads `tokens_[pos_]` without advancing.
pure observation — calling it 10 times in a row returns the same token.

`consume()` is not `const`: it returns `tokens_[pos_]` and increments
`pos_`. a side-effecting read.

`peek()` is used when a decision is needed before commitment:
`at_STRING("location")` in `parse_server_block` checks before consuming.
`consume()` is used when the token is known to be correct and can be
advanced past.

both are safe at stream exhaustion because `tokenise()` appended END.
`tokens_[pos_]` always refers to a valid token.

### expect — the `type_name` lambda and operator+

`expect()` consumes a token and throws if the type mismatches.
it needs to produce a human-readable error naming both expected and
received types.

the lambda inside `expect`:

```cpp
auto type_name = [](TokenType tp) -> std::string { ... };
```

`[]` — captures nothing. it is a pure function of its argument.
it could be a static helper function. it is a lambda because it
exists only here, converts an enum to a string for error formatting,
and has no other caller.

the parameter `tp` rather than `type`: `type` is already in scope
as the `expect` parameter. shadowing it would compile but mislead.

the error message is built by `operator+` on `std::string`.
`std::string` overloads `+` to mean concatenation — not arithmetic.
the chain works left-to-right; each `+` returns a new `std::string`.
the leftmost operand must be `std::string` or the chain fails:
`"literal" + "literal"` in C++ is pointer arithmetic, not string
concatenation. `std::to_string(t.line)` returns `std::string`,
anchoring the left side of the chain.

### expect_STRING vs expect(TokenType::STRING)

these are not equivalent.

`expect(TokenType::STRING)` on mismatch says: "expected string".
`expect_STRING()` on mismatch says: "expected directive value".

the difference is operator-facing versus implementation-facing.
at the call site where `expect_STRING()` is used, the grammar
demands a directive value or identifier. the error should say
what the grammar position requires, not what the token type is.
a config operator does not know what `STRING` means; they do know
what "directive value" means.

---

## dispatch — the if-chain

`parse_server` and `parse_location` dispatch to specific directive
parsers via a linear chain of string comparisons:

```cpp
if (name.value == "listen")  { parse_listen(s);  return; }
if (name.value == "root")    { parse_root(loc);  return; }
...
```

alternatives considered:

`std::map<std::string, std::function<...>>` — O(log n) dispatch,
removes the chain. adds indirection: `std::function` involves
allocation and a virtual call. adds noise: the map must be
constructed and managed. for a parser that executes once at startup
with ~10 directive names, the O(n) if-chain is identical in practice.

switch on a hash — fast but requires a hash collision-free mapping
determined at compile time. fragile under directive additions.

the if-chain is chosen for 3 properties: readable, debuggable
(each branch is visible), and correct. performance is irrelevant
at startup.

### the `name.type != STRING` guard in dispatch

```cpp
if (name.type != TokenType::STRING)
    throw std::runtime_error(...);
```

by the time `parse_server` is called, `parse_server_block` has
already ruled out RBRACE and END via the while condition, and
`at_STRING("location")` was false. so peek is STRING. the guard is
technically redundant given the call contract.

it is retained as a defensive assertion: if the call contract
breaks — if someone adds a code path that reaches `parse_server`
with a non-STRING token — the guard converts a silent wrong
assumption into a visible, located error. the cost is 1 comparison.

---

## defaults applied at block entry

the struct is initialised with defaults before the directive loop.
any directive present in config overrides.
absence of a directive → default value persists.

```cpp
ServerConfig s;
s.client_max_body_size = 1048576; // 1m
```

```cpp
Location loc;
loc.allowed_methods = {HttpMethod::GET, HttpMethod::POST, HttpMethod::DELETE};
loc.autoindex       = false;
loc.upload_enable   = false;
```

fields with no meaningful default (`root`, `cgi_extension`, etc.)
are empty strings by struct construction. the validator enforces
mandatory presence where required — the parser does not.

### allowed_methods — clear before insert

the default is `{GET, POST, DELETE}` — all methods, representing
"no restriction stated". the directive `allowed_methods GET;` means:
restrict to exactly GET. if the parser inserted into the existing
set, the result would always include the defaults regardless of
what the config stated. clearing first makes the directive an
override, not an additive operation.

### autoindex

when a URI maps to a directory and no index file exists,
`autoindex on` instructs the server to generate an HTML directory
listing. `autoindex off` (default) returns 403 instead.
off by default: exposing filesystem structure is a security concern
and should require explicit operator opt-in.

---

## directive consumption contract

`parse_server` consumes the directive name token, then dispatches.
the specific parser (`parse_listen`, `parse_root`, etc.) enters
with `pos_` pointing at the 1st value token. it consumes values
and the terminating semicolon, then returns.

the name token is spent as the dispatch decision — it has no meaning
after dispatch. consuming it again inside the specific parser would
be redundant and would create a hidden coupling: every specific
parser would need to know to skip its own name. that is a trap for
every future addition.

violating this contract — specific parser consuming its own name —
produces off-by-1 token errors, which are silent until the wrong
field is populated.

---

## interpretation leaves

the 3 leaf functions convert STRING token values to typed values.
they are the single enforcement points for numeric correctness.

### parse_host_port

grammar: `host_port = port | host, ":", port`

colon presence distinguishes the 2 forms. C++17 if-init scopes
`pos` to the branch where it is meaningful:

```cpp
if (auto pos = t.value.find(':'); pos != std::string::npos)
```

`pos` does not leak into the surrounding scope. without C++17
init-statement, it would be declared before the `if`, visible
where it has no meaning.

### parse_port

`stoi` over `stoul`: `stoi` throws `std::invalid_argument` on
non-numeric input. `stoul` silently accepts leading whitespace
and some edge inputs. `stoi` is the stricter conversion.

valid range [1, 65535]: `uint16_t` admits 0, but port 0 is not
a valid service port. the range check is a semantic constraint
layered on top of the type.

### parse_size

grammar: `size = digit, { digit }, [ size_suffix ]`
size_suffix: `k/K/m/M/g/G`

`stoull` over `stoul`: on 32-bit platforms `size_t` is 32 bits.
`stoul` on a 32-bit platform would truncate before the cast to
`size_t`. `stoull` gives 64-bit precision for the intermediate
value, then the cast carries the full value or the check catches
overflow.

`static_cast<unsigned char>` before `isdigit`: `char` may be
signed. passing a negative value to `isdigit` is undefined
behaviour (the argument must be representable as `unsigned char`
or be `EOF`). the cast is not defensive style — it is required
correctness.

---

## error format

all parse errors follow:

```
[config] line <N>: <message>
```

`N` is carried on every token from the tokeniser.
the message states what the grammar position required,
not what the implementation expected.