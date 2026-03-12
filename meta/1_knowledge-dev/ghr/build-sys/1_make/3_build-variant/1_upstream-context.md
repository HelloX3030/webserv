# upstream context


## why this precedes the architectures

Make's variant mechanisms are workarounds for missing abstractions.
understanding what Make lacks — by seeing systems that have it —
illuminates why the workarounds take the forms they do.


---


## Shake: variants as first-class values

in Shake, a variant is a Haskell value — a record or algebraic
data type. a rule parameterised by a variant is a function closed
over that value:

```haskell
data Variant = Release | Debug | Asan deriving (Show, Eq)

compileFlags :: Variant -> [String]
compileFlags Release = ["-O2"]
compileFlags Debug   = ["-DDEBUG=1", "-g", "-O0"]
compileFlags Asan    = ["-fsanitize=address", "-g"]

linkFlags :: Variant -> [String]
linkFlags Asan = ["-fsanitize=address"]
linkFlags _    = []
```

the type system enforces the separation that Make enforces only
by programmer discipline: variant-specific flags cannot reach the
wrong build context without a type error. no target-specific
variable machinery, no define/endef workaround — the variant is
threaded explicitly as a value through every function. composition
is transparent and type-checked.


---


## Cargo: variants normalised into the tool

Cargo's profile system (dev, release, bench, test) is precisely
the variant concern, lifted out of user build descriptions and
into the build tool itself. you declare which profile you want;
Cargo manages separate output directories, flag sets, and
artifact names.

the pathology of triplicating rules does not arise because the
tool owns the variant concern. this is the upstream resolution:
recognise that variant management is a recurring, domain-general
problem, and solve it once in the build tool rather than in every
project's Makefile.


---


## what Make lacks

Make does not solve the variant problem because it is not a
build-lifecycle manager — it is a general-purpose rule evaluator.
its design philosophy is maximal generality at the cost of
domain-specific conveniences.

the architectures that follow are strategies for representing
variant configuration within Make's constraint: no first-class
variant values, no tool-level profile management, only variables,
conditionals, target-specific scoping, and text manipulation.


---


## sources

Mitchell, N. "Shake Before Building." ICFP 2012.
https://dl.acm.org/doi/10.1145/2364527.2364538
the typed alternative; demonstrates how first-class values
eliminate target-specific variable machinery.

Cargo reference: profiles.
https://doc.rust-lang.org/cargo/reference/profiles.html
the normalised, tool-level solution.
