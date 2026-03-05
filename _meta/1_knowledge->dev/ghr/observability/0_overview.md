# observability

the capacity to determine the internal state of a running system
from its external outputs.

coined by Rudolf Kalman (1960) in control theory: a dynamical system
is observable if its current state can be reconstructed from a finite
sequence of outputs. software engineering adopted the term to name the
same property at the level of programs and services.

observability is a property a system either possesses or lacks.
tools make it possible to exercise that capacity — they do not
constitute it.


---


## relation to serialisation

to observe state is to externalise it: convert in-memory structure
to a form that can be read, transmitted, or recorded.

serialisation is the necessary upstream mechanism.
observability is the telos of a particular class of serialisation —
the class whose purpose is inspection of runtime state, rather than
persistence or wire transmission.


---


## decomposition

canonical concerns:

- logging: timestamped record of discrete events as they occur
- tracing: execution flow through functions, services, or systems
- metrics: quantitative aggregates over time (counts, rates, gauges)
- debugging: ad-hoc, interactive inspection of state at a point in time

these answer distinct questions:

    logging   → what happened, and when?
    tracing   → how did execution reach this state?
    metrics   → at what rate, how often, with what magnitude?
    debugging → what is the current value of X?

they are not alternatives — a system with full observability has all 4.


---


## the representation layer

all 4 concerns share a prerequisite: state must be renderable as a
string or structured record. this rendering is not observability —
it is its substrate.

`to_string` in C++, `Show` in Haskell, `Debug`/`Display` in Rust:
representation primitives. they enable observability; they are not
instances of it.


---


## other languages

agda:
    "observational equivalence" is a precise type-theoretic concept:
    2 terms are observationally equivalent when no context can
    distinguish them. upstream, mathematical usage — a property of
    a formal system, not a runtime instrument.

haskell:
    `Show` typeclass (ghc-derivable) is the canonical representation
    primitive. runtime observability: `Debug.Trace` for ad-hoc
    inspection; `fast-logger`, `katip` for structured logging;
    `opentelemetry-haskell` for distributed tracing.

rust:
    `std::fmt::Display` (human-facing) and `std::fmt::Debug`
    (programmer-facing, `#[derive(Debug)]`) as representation
    primitives. `tracing` crate is the ecosystem standard —
    structured, async-aware, covering logs and spans in 1 system.