# debug logging — C++

debug logging is observability exercised at development time:
rendering runtime state as human-readable output to assist the
programmer in understanding system behaviour.

C++ provides no standard logging facility. the programmer assembles
one from primitives. the idioms below are the stable building blocks.


---


## conditional compilation

```cpp
#ifdef DEBUG
    // diagnostic code
#endif
```

`#ifdef` is a preprocessor directive — evaluated before the compiler
sees the code. the enclosed block is either included or erased at
the text-substitution stage, before parsing or type-checking occur.

the `DEBUG` symbol is defined at compile time, not at runtime:

```
c++ -DDEBUG main.cpp
```

consequence: debug builds and release builds are genuinely distinct
translation units. the diagnostic code has zero cost in release —
it does not exist in the binary. this is not an optimisation hint
(as with `if constexpr`) but literal absence.

in webserv: `log.cpp` guards the title-length assertion with `#ifdef DEBUG`.
the debug mode for the full program — `log::log` calls in the event loop,
`to_string` on handlers — is gated the same way.


---


## stderr vs stdout

2 output streams exist for a reason:    // in Unix-based systems

- `stdout` (`std::cout`): program output — the data the program produces.
- `stderr` (`std::cerr`): diagnostic output — events, errors, state.

the distinction matters operationally:

```
./server 2>/dev/null          # suppress diagnostics, keep output
./server 2>debug.log          # capture diagnostics separately
./server > out.txt 2> err.txt # separate both
```

`stderr` is unbuffered by default. `stdout` is line-buffered (tty)
or fully buffered (pipe/file). under a crash, buffered stdout output
may be lost; stderr output is not.

in webserv: `log::log` sends `LogType::ERROR` to `std::cerr`,
all other types to `std::cout`. this respects the distinction.


---


## the representation substrate

all debug output requires state to be renderable as a string.
this is not logging — it is the necessary precondition of logging.

in webserv: `to_string(ServerConfig)`, `to_string(Location)`,
`to_string(ListenAddress)` in `Config.cpp`. also `to_string(LogType)`
and `EpollHandler::to_string()` on the virtual interface, present
solely because the debug path in `WebServ::run()` requires it.

C++17 has no reflection — these functions manually enumerate struct
fields. a field added to a struct without updating its `to_string`
produces no compiler error; the omission is silent.


---


## a minimal logging function

```cpp
void log(std::string title, std::string msg, std::string value,
         LogType type)
{
    // centre title in fixed-width column
    // route to cout or cerr by type
    // format: [   TITLE   ] msg: value
}
```

the pattern in webserv's `log::log`:
- fixed-width title column (20 chars, centred) — visual alignment
  across heterogeneous call sites.
- `msg` and `value` separated — allows structured queries on log output
  (grep by message pattern, not value).
- `LogType` enum selects stream and format variant.

`to_string(log::LogType)` renders the enum for error messages —
the function itself logs when an unknown `LogType` is passed.


---


## other languages

agda:
    no runtime execution in the proof-relevant sense.
    the analog is definitional equality and normalisation:
    a term can be inspected by reducing it. no concept of
    "debug output" applies.

haskell:
    `Debug.Trace.trace :: String -> a -> a` — inserts a `putStrLn`
    as a side effect into a pure computation. deliberately impure,
    marked as unsafe. `traceShow` uses the `Show` instance directly.
    for structured logging: `fast-logger`, `katip`.
    conditional compilation: `CPP` language extension + cabal flags.

rust:
    `eprintln!` writes to stderr — the idiom equivalent of
    `std::cerr <<`. `dbg!(expr)` macro prints file, line, and
    the `Debug` representation of any expression, then returns the
    value — usable inline without restructuring code.
    `log` crate: facade defining `debug!`, `info!`, `warn!`, `error!`
    macros with no output backend. `env_logger` is the standard
    backend: output controlled by `RUST_LOG` env var at runtime,
    no recompilation needed to adjust verbosity.
    `tracing` crate extends this with structured spans.