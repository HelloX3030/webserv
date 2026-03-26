organisation within cpp/
Exceptions span control flow, object model (stack unwinding, destructors), and type system (exception specifications)



# exceptions

a mechanism for non-local transfer of control from a failure site
to a handler, bypassing all intermediate call frames.


---


## origin

not a C++ invention. the upstream source is CLU — a language
designed by Barbara Liskov at MIT, 1975. CLU introduced exceptions
as a typed, first-class mechanism: a function could `signal` a named
condition; its caller could `except` on it by name.

the core semantics — non-local control transfer, named exception
types, separation of detection site from handling site — flow from
CLU into Ada (1983), C++ (late 1980s, standardised in C++98), Java,
C#, and most subsequent languages.

2 deliberate departures C++ made from CLU:

checked vs unchecked:
    CLU exceptions were checked — the type system tracked which
    exceptions each function could signal, and callers were required
    to handle or declare them. C++ exceptions are unchecked: no
    compiler enforcement at call sites. Java restored checking;
    the industry has largely rejected mandatory checked exceptions
    as too burdensome at scale. Haskell and Rust use typed return
    values (`Either`, `Result`) instead, making failure visible in
    the type without a separate exception mechanism.

resumable vs non-resumable:
    CLU's `signal` was potentially resumable — the handler could
    return a value and allow the signalling function to continue.
    C++ `throw` is strictly non-resumable: the stack unwinds and
    the throw site is gone. Stroustrup chose this deliberately;
    resumable exceptions complicate the mental model significantly.


---


## mechanics

```cpp
throw std::runtime_error("message");
```

`throw` constructs the exception object and initiates stack
unwinding. each frame on the call stack is exited in order:
destructors of all local objects in each frame are invoked.
this is the designed interaction with RAII — resource cleanup
happens automatically during unwinding, without explicit cleanup
code in intermediate frames.

unwinding continues until a matching `catch` is found:

```cpp
catch (const std::exception& e) { ... }
```

if no matching `catch` exists anywhere in the stack,
`std::terminate` is called. the program ends.

catching by `const&` is required to avoid object slicing:
a `runtime_error` caught as `std::exception` by value would
lose the concrete type. by reference, the full object is
accessible and `what()` returns the correct message.


---


## the exception hierarchy

```
std::exception
    ├── std::logic_error      — programming error; detectable by
    │       ├── invalid_argument    inspection. should not occur.
    │       ├── out_of_range
    │       └── ...
    └── std::runtime_error    — contingent on runtime environment.
            ├── range_error         file absent, input malformed,
            ├── overflow_error      network failure.
            └── ...
```

the branch distinction is semantic. both propagate identically.
choosing `runtime_error` vs `logic_error` declares the nature of
the failure at the throw site.


---


## noexcept

```cpp
void f() noexcept;
```

declares that `f` will not throw. if it does, `std::terminate`
is called immediately — no unwinding. the compiler may use this
to generate more efficient code (no unwind tables for this frame).

relevant for: destructors (implicitly `noexcept` since C++11),
move constructors, performance-critical paths.


---


## other languages

agda:
    no exceptions. failure is partiality — a function that can
    fail returns `Maybe A` or `Either E A`. the type encodes the
    possibility of failure. there is no mechanism for non-local
    control transfer; all failure is explicit in the return type.

haskell:
    2 mechanisms coexist. pure code uses `Maybe`/`Either` (failure
    as value — no control flow). `IO` code can throw and catch
    exceptions (`Control.Exception`), but this is an effect and
    must be in the `IO` monad. the purity boundary is respected.
    `SomeException` is the root type, analogous to `std::exception`.

rust:
    no exceptions. recoverable failure: `Result<T, E>` — explicit
    in the type, propagated with `?` operator. unrecoverable failure:
    `panic!` — unwinds the thread, analogous to an uncaught exception.
    the language enforces the distinction between recoverable and
    unrecoverable at the type level, not by convention.
