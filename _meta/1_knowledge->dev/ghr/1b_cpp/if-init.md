# C++17 if-init — scoped variable introduction

---

## problem: scope leakage

pre-C++17, introducing a variable for a condition leaked it into the
enclosing scope:

```cpp
auto pos = s.find(':');
if (pos != std::string::npos)
{
    // use pos — meaningful here
}
// pos STILL EXISTS here — no longer meaningful
// can accidentally reuse, shadow, or confuse
```

`pos` outlives its semantic relevance. it exists where it has no
business existing.

this violates C++ Core Guidelines ES.21 and ES.22:
- ES.21: don't introduce a variable before you need to use it
- ES.22: don't declare a variable until you have a value to initialize it

the underlying principle: a name should exist exactly where it is
meaningful, and nowhere else.

---

## solution: if-init (C++17)

```cpp
if (auto pos = s.find(':'); pos != std::string::npos)
{
    // pos exists here
}
else
{
    // pos exists here too (both branches)
}
// pos DOES NOT EXIST here — scope ended
```

syntax: `if (init-statement; condition)`

the semicolon separates:
- init-statement: variable declaration (or any statement)
- condition: the boolean test

the variable's scope is the entire if-else construct, then it dies.

---

## logical necessity / telos

why is this required, not merely convenient?

### 1. scope = lifetime of meaning

a variable represents a value. that value has a context where it
matters. outside that context, the name is noise — or worse, a trap.

```cpp
auto pos = s.find(':');
if (pos != std::string::npos) { /* ... */ }

// 50 lines later, someone writes:
auto pos = t.find('-');  // ERROR: redeclaration
// or worse, they use the stale `pos` thinking it's fresh
```

leaking scope creates: name collisions, stale value bugs, cognitive
load (tracking what's "really" live).

### 2. resource determinism

C++ ties resource lifetime to scope (RAII). tighter scope = earlier
destruction = deterministic cleanup.

```cpp
if (auto lock = mutex.try_lock(); lock)
{
    // mutex held
}
// mutex released HERE, not at function end
```

### 3. expression of intent

code should say what it means. if a variable exists only for a
conditional check, the syntax should show that:

```cpp
// old: pos declared, then tested — 2 separate statements
// reader must infer: "ah, pos is only for this if"

// new: declaration INSIDE the if
// reader sees: "pos exists for this if, period"
```

---

## anatomy of the idiom

```cpp
if (auto pos = s.find(':'); pos != std::string::npos)
```

`auto pos = s.find(':')` — init-statement (declaration)
`;` — separator (required, not optional)
`pos != std::string::npos` — condition (bool)

without the semicolon, the parser cannot distinguish:

```cpp
if (auto x = foo())      // x IS the condition (contextual bool)
if (auto x = foo(); x)   // x is declared, then tested
```

---

## equivalence to for-loop scoping

```cpp
for (int i = 0; i < n; ++i)
{
    // i exists here
}
// i does not exist here
```

the for-loop has always scoped its init to its body. C++17 if-init
extends the same discipline to conditionals.

---

## practical patterns

resource acquisition:

```cpp
if (auto file = std::fstream("data.txt"); file.is_open())
{
    // use file
}
// file closed (destructor runs)
```

map lookup:

```cpp
if (auto it = map.find(key); it != map.end())
{
    // use it->second
}
```

pointer validity:

```cpp
if (auto* ptr = dynamic_cast<Derived*>(base); ptr != nullptr)
{
    // use ptr
}
```

with structured bindings (C++17):

```cpp
if (auto [it, inserted] = set.insert(value); inserted)
{
    // insertion succeeded
}
```

---

## also applies to switch

```cpp
switch (auto ch = get_char(); ch)
{
    case 'a': /* ... */ break;
    case 'b': /* ... */ break;
    default:  /* ... */ break;
}
// ch does not exist here
```

---

## summary

| aspect | pre-C++17 | C++17 if-init |
|--------|-----------|---------------|
| scope | leaks to enclosing block | confined to if-else |
| intent | implicit (reader infers) | explicit (syntax shows) |
| safety | stale reuse possible | name dies when irrelevant |
| pattern | for-loops only | if, switch, for |

the telos: names exist where they mean something, and cease to exist
when they don't.

---

## scoping in other languages

### Agda

dependent pattern matching scopes variables to their clause:

```agda
parse : String → Maybe (String × String)
parse s with break (== ':') s
... | (host , _ ∷ port) = just (host , port)
... | _                 = nothing
```

variables bound by patterns exist only in that clause. scope is
structural, not a bolted-on feature.

### Haskell

bindings are inherently scoped by expression structure:

```haskell
case findIndex (== ':') s of
  Just pos -> splitAt pos s   -- pos scoped to this branch
  Nothing  -> ("0.0.0.0", s)
```

or with `let...in`:

```haskell
let pos = findColon s in if isJust pos then ... else ...
```

no special syntax needed. all bindings are expressions with inherent
scope. the language was designed this way.

### Rust

`if let` provides scoped binding:

```rust
if let Some(pos) = s.find(':') {
    // pos exists here
}
// pos does not exist here
```

designed into the language from the start. cleaner syntax than C++
because Rust had no legacy grammar to maintain.

---

## why C++ syntax is awkward

C++ accretes features onto 1970s C grammar. the semicolon-in-if is
pragmatic: it reuses the existing for-loop pattern rather than
inventing new syntax.

```cpp
for (init; condition; increment)  // established pattern
if  (init; condition)             // C++17 extension
```

cost: readability. benefit: parser compatibility, familiar structure.

languages designed later (Rust, Haskell, Agda) express scoped
bindings naturally because they were built with this in mind. C++
retrofits sanity onto legacy — the syntax reflects that constraint.