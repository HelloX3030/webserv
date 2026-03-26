# configuration vs state

## ontology

2 categories of data a component holds:

**configuration**: values injected from outside, constant for lifetime,
representing constraints or context the component cannot determine itself.

**state**: values that evolve during operation, determined by the component's
own logic and inputs.


---


## the distinction

| property | configuration | state |
|----------|---------------|-------|
| origin | external (caller, environment) | internal (derived from operation) |
| mutability | constant after initialisation | changes during lifetime |
| knowledge | component cannot know it | component computes it |
| initialisation | must be provided | has determinate initial value |


---


## example: HttpRequestFrontend
```cpp
HttpRequestFrontend(size_t max_body_size)
    : max_body_size_(max_body_size)  // configuration
    , phase_(ParsePhase::REQUEST_LINE)  // state
    , buffer_()  // state
    , body_remaining_(0)  // state
```

`max_body_size_`: the parser cannot determine this. the server configuration
knows what limit to enforce. injected, constant.

`phase_`: the parser knows exactly where it starts. `REQUEST_LINE` is not
a choice — it is the logical starting point. determined, mutable.

`buffer_`: starts empty by definition. no bytes have arrived. determined.


---


## the principle

constructor parameters exist for injection of external dependencies.

if a value:
- has a known initial state by definition → hardcode it
- depends on external context → require it as parameter

passing internally-determined values as parameters is ceremony without
information. "start at REQUEST_LINE" is not a choice the caller makes.


---


## connection to dependency injection

configuration-as-parameter is minimal dependency injection.
the component declares: "I need this from outside."
the caller provides it.

this makes dependencies explicit. a component's constructor signature
documents what external knowledge it requires.


---


## in type theory

configuration corresponds to parameters in a parameterised module or functor.
state corresponds to internal definitions computed from those parameters.
```
module Parser (maxSize : Nat) where
  -- maxSize is configuration
  -- internal definitions are state
```

the module is indexed by its configuration. different configurations
yield different module instances.
