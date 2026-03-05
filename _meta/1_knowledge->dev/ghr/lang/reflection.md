# reflection

a language's capacity to treat its own structure as data —
to inspect, reason about, or generate code from types at the
meta-level.

2 distinct capabilities, often conflated:

- compile-time (static): the compiler inspects type structure
  and generates code before the program runs.
- runtime (dynamic): a running program inspects its own type
  information during execution.

a language can have one, both, or neither. the 2 are logically
independent — having one does not imply the other.


## the problem reflection solves

given a struct with n fields, producing a human-readable
representation requires enumerating those fields. without
reflection, this enumeration is manual — written by the
programmer, maintained by the programmer, and silently wrong
when the struct changes but the enumeration does not.

the compiler knows the fields at compile time. the question is
whether it exposes that knowledge to the programmer, or uses it
itself for code generation.


## C++: RTTI — the runtime mechanism

C++ has a runtime type facility: RTTI (Runtime Type Information).
it provides 2 things.

`typeid(x)` — returns a `std::type_info` object representing
the type of `x`. the only useful method on it is `.name()`,
which returns a compiler-mangled, implementation-defined string
of the type name. not standardised: GCC returns `"11ServerConfig"`,
MSVC returns `"struct ServerConfig"`. not portable, not parseable.

`dynamic_cast<T*>(ptr)` — attempts a downcast along an
inheritance hierarchy at runtime. returns `nullptr` on failure.
requires at least 1 virtual function in the hierarchy (i.e. a
vtable must exist for the runtime type to be known).

both require a polymorphic type — a class with at least 1
virtual function — to operate on the dynamic (runtime) type.
on a plain struct with no virtual functions, `typeid` returns
the static (compile-time) type only, and `dynamic_cast` is
unavailable.

critically: RTTI tells you what type something is. it tells
you nothing about what fields that type has. it cannot enumerate
members. it does not solve the manual enumeration problem.

RTTI is a runtime identity mechanism, not a structural
introspection mechanism. the distinction matters.


## C++17: no static reflection

C++17 has no mechanism for compile-time struct introspection.
the compiler cannot expose field membership to the programmer
for code generation. template metaprogramming can reason about
types — their relationships, their properties — but not about
the members of an arbitrary struct.

workarounds exist but are all manual or macro-based:

- `X_MACRO` patterns enumerate fields via macro expansion.
  fragile and unreadable.
- `boost::pfr` (Precise and Flat Reflection) uses structured
  bindings heuristics to iterate aggregate members. works for
  simple aggregates; breaks under inheritance, private members,
  or non-aggregate types.

none are general. all are workarounds for an absent language
feature.


## C++26: P2996

static reflection was voted into C++26 at the June 2025 Sofia
meeting of WG21 (bloomberg/clang-p2996, github.com, June 2025;
learnmoderncpp.com, July 2025).

the core mechanism: a new operator `^^` that produces a
`std::meta::info` value — an opaque handle representing any
program entity (type, member, function, namespace). consteval
functions in `<meta>` query these handles:

```cpp
for (constexpr std::meta::info m :
         std::meta::nonstatic_data_members_of(^^ServerConfig))
{
    std::cout << std::meta::name_of(m) << "\n";
}
```

this loop iterates over every non-static data member of
`ServerConfig` at compile time. adding a field to `ServerConfig`
automatically makes it visible here — no manual maintenance.

code generation (P3294, token injection) is deferred to C++29.
P2996 covers introspection; generation of new declarations at
compile time is a separate problem.

experimental implementation: bloomberg's clang fork
(github.com/bloomberg/clang-p2996), also on Compiler Explorer.


## across languages

### Agda

full compile-time reflection via `Agda.Builtin.Reflection`.
the programmer can quote terms and types, inspect their AST,
and write tactics that manipulate them. the meta-level is the
same language as the object level — not a separate facility.
this is a consequence of dependent types, where the boundary
between types and terms, compile-time and runtime, is formally
fluid and tractable.

### Haskell

`deriving Show` — GHC inspects the algebraic data type
definition at compile time and generates a `Show` instance
automatically. adding a field updates the derived representation
automatically: zero manual maintenance.

Template Haskell provides the general underlying mechanism:
compile-time access to the AST, enabling arbitrary code
generation from type definitions. `deriving` is the common
case made ergonomic.

### Rust

`#[derive(Debug)]` — a procedural macro that inspects the
struct definition at compile time and generates a `Debug` impl.

`Display` is intentionally not derivable: it requires semantic
intent from the programmer. this distinction — `Debug`
derivable, `Display` not — is a deliberate design decision:
`Debug` is mechanical (complete field dump), `Display` is
meaningful (user-facing representation).

### C++17

no static reflection. RTTI exists for runtime identity only,
not structural introspection. static reflection (P2996) voted
into C++26 at the 2025 Sofia WG21 meeting.


## concrete example: webserv Config.cpp

`to_string(ServerConfig)` manually enumerates all fields of
`ServerConfig`: listen addresses, server names, body size limit,
error pages, locations. if a field is added to `ServerConfig`
in `Config.hpp`, the compiler emits no warning. the rendered
output silently omits the new field.

mitigation: the file-level comment in `Config.cpp` states the
liability explicitly:

    maintenance liability: each to_string function manually
    enumerates its struct's fields. C++17 has no reflection —
    the compiler cannot detect a field added to a struct but
    omitted here. divergence is silent. update to_string
    whenever the corresponding struct changes.

in Rust or Haskell this liability does not exist. in C++26 it
is eliminable via `nonstatic_data_members_of` from P2996.