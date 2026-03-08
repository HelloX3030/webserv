# auto — type deduction

---

## what it is

`auto` is a keyword. it instructs the compiler to deduce the type
from the initializer expression. introduced in C++11.

```cpp
auto x = 42;          // int
auto s = "hello";     // const char*
auto v = std::vector<int>{1, 2, 3};  // std::vector<int>
```

the variable has a concrete type at compile time. `auto` is not
dynamic typing — it is static typing with inference.

---

## problem: mandatory redundancy

pre-C++11:

```cpp
std::map<std::string, std::vector<int>>::iterator it = m.begin();
```

the compiler knows what `m.begin()` returns. the declared type on
the left must match it (or implicitly convert). you cannot declare
a different type — the compiler enforces this.

so the declaration carries no new semantic information. it is a
mandatory echo of what the compiler already determined from the RHS.
this is redundancy: forced repetition without expressive power.

if the container type changes, every declaration site must update
manually. the type on the left must track the type on the right.

---

## solution: let the compiler state what it knows

```cpp
auto it = m.begin();
```

the compiler determines the type. you don't repeat it.

---

## comparison to other languages

### Agda / Haskell — inference + optional specification

```haskell
foo :: Int -> Int -> Int   -- optional: specification, not echo
foo x y = x + y
```

the signature constrains what the implementation CAN be. it is a
contract: the implementation must satisfy it. if omitted, the
compiler infers the most general type.

NOT redundancy — signature restricts the space of valid implementations.

C++ pre-auto had no such optionality. you HAD to write the type,
and it HAD to match what the RHS produced. no constraint, no
specification — just mandatory echo.

C++ `auto` ≈ Haskell/Agda default (inferred, no signature).
C++ explicit type ≠ Haskell signature (echo vs specification).

### Rust — inference with optional annotation

```rust
let x = 42;           // inferred: i32
let y: i32 = 42;      // explicit annotation (optional here)
```

similar to C++ `auto`. inference by default. annotation when needed
for disambiguation or documentation.

### Guile/Scheme — dynamic typing (different category)

```scheme
(define x 42)
(set! x "hello")   ; valid: x can hold any type
```

no static types. variables do not have types — values do, at runtime.
not comparable to `auto`. `auto` is static inference. Scheme has no
static type to infer.

---

## summary of distinctions

| language | mechanism | type lives... |
|----------|-----------|---------------|
| C++ pre-auto | mandatory declaration | stated explicitly, must match RHS |
| C++ auto | inference | deduced at compile time |
| Haskell/Agda | inference + optional signature | inferred, or constrained by signature |
| Rust | inference + optional annotation | inferred, or annotated |
| Scheme/Guile | dynamic | attached to values at runtime |

---

## how auto works

the compiler examines the initializer expression and assigns the
deduced type to the variable. rules follow template argument
deduction.

```cpp
auto a = 42;           // int (literal type)
auto b = 42.0;         // double
auto c = 42.0f;        // float
auto d = &a;           // int*
```

`auto` strips top-level const and references by default.
use `auto&`, `const auto&`, `auto&&` to preserve them:

```cpp
const int x = 10;
auto a = x;        // int (const stripped)
const auto b = x;  // const int
auto& c = x;       // const int& (reference preserved)
```

---

## when to use

iterators:
```cpp
auto it = container.begin();
```

lambdas (type is unnameable):
```cpp
auto f = [](int x) { return x * 2; };
```

factory returns:
```cpp
auto ptr = std::make_unique<Widget>();
```

when the type is not obvious from context, explicit typing may
clarify intent:

```cpp
auto x = foo();      // unclear: reader must find foo()'s signature
Widget x = foo();    // immediately clear
```

use `auto` when the type is obvious or unimportant.
use explicit types when the type IS the point.