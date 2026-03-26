# name resolution

## ontology

a name is a symbol that stands for something else.
name resolution is the process of determining what a name denotes.

in natural language: hearing "the river" and determining which river.
in programming: encountering `x` and determining which declaration it refers to.
in logic: seeing a variable and determining its binding.


---


## the resolution process

given a name at a use site, the language must answer:
where was this name introduced, and what does it mean here?

this requires:
1. **scope rules**: which declarations are visible at a given point
2. **lookup algorithm**: how to search among visible declarations
3. **disambiguation**: what happens when multiple candidates exist


---


## qualified vs unqualified

**unqualified**: the bare name. `x`, `ParsePhase`, `map`.
resolution searches through enclosing scopes according to language rules.

**qualified**: the name with explicit path. `std::map`, `HttpRequestFrontend::ParsePhase`.
resolution follows the path directly — no search.

qualification is explicit navigation. unqualification delegates to scope rules.


---


## scope

a scope is a region where a set of names is visible.

scopes nest: inner scopes may shadow outer scopes.
languages differ in what creates scope (blocks, functions, classes, modules, files).
```
global scope
└── namespace scope
    └── class scope
        └── function scope
            └── block scope
```

lookup typically proceeds inside-out: search innermost scope first,
then enclosing scopes, until found or exhausted.


---


## manifestations

**C++**: complex lookup — unqualified, qualified, argument-dependent (ADL),
template-dependent. class scope includes base classes. namespaces can be
reopened. `using` declarations inject names.

**Haskell**: module-based. unqualified names found via imports or local binding.
qualified via module prefix (`Data.Map.lookup`). no inheritance hierarchy.

**Agda**: explicit imports. names qualified by module path.
no implicit scope injection. highly explicit.

**Rust**: module tree with `use` for imports. `::` for qualification.
`self`, `super`, `crate` for relative paths. clear hierarchy.


---


## connection to binding

lookup finds the *declaration*. binding connects the use to the declaration.
in type theory: the typing context Γ records bindings; lookup searches Γ.
```
Γ = x : Int, y : Bool

lookup(Γ, y) = Bool
```

the context is the formalisation of "what names are in scope and what do they mean".


---


## references

Pierce, B. — Types and Programming Languages, Ch. 6 (nameless representation
addresses lookup complexity).

C++ standard §6.5 (name lookup).
