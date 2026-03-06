# log system — first principles design


---


## the irreducible dimensions of a log message

a log message has 4 orthogonal axes. none can be collapsed into
another without information loss:

    severity    — how significant is this event?
    source      — which component emitted it?
    content     — what happened? (message + optional value)
    destination — where does the output go?

destination is *derived* from severity. that it routes to stderr or
stdout is a consequence of how significant the event is — not an
independent property the call site should specify.

this derivation is the key structural insight. once it is respected,
the enum design follows necessarily.


---


## severity levels

```
DEBUG   compile-time gated. programmer inspection only.
        absent from release binary entirely.

INFO    normal operational events.
        connection accepted, request parsed, config loaded.

WARN    unexpected but recovered.
        malformed header ignored, retry succeeded.
        the system continues correctly — but something was off.

ERROR   failure. the system could not do what was asked.
        recoverable (bad request → 400) or not (socket error).
```

4 levels. 

why not fewer?

    DEBUG vs INFO: DEBUG is a compile-time category, not a runtime one. 
    collapsing them would mean either: debug output in release
    binaries, or suppressing INFO in debug builds. neither is acceptable.

    WARN vs ERROR: a recoverable anomaly is not a failure. collapsing
    them makes the output untrustworthy — every warning looks like an error.

why not more?

    FATAL, CRITICAL, NOTICE, TRACE — these either collapse into the
    4 above, or are operational concerns (syslog priority levels,
    distributed tracing) irrelevant at this scale. add them when
    the system demands them.


---


## interface

```cpp
namespace log {

enum class Level { DEBUG, INFO, WARN, ERROR };

void log(Level, const char* src, std::string msg);
void log(Level, const char* src, std::string msg, std::string value);
void log(Level, const char* src, std::string msg, std::size_t i);

} // namespace log
```

`src` is a `constexpr const char*` identifier — the same title
strings already defined in `defines.hpp`. they become the structured
`source` field. call sites do not change.

routing is internal to the implementation:

    ERROR → stderr
    all others → stdout

call sites never specify a destination. they specify significance.
the system derives the rest.

the `LIST` format disappears: argument order is the call site's
responsibility. if the index should precede the message, pass it
that way. a format preference is not a log type.


---


## what changes at call sites

current:

```cpp
log::log(CONNECTION, "bytes read", std::to_string(n), log::LogType::DEFAULT);
log::log(WEB_SERV, e.what(), log::LogType::ERROR);
```

proposed:

```cpp
log::log(log::Level::INFO,  CONNECTION, "bytes read", n);
log::log(log::Level::ERROR, WEB_SERV,   e.what());
```

`LogType::DEFAULT` → `Level::INFO`. the unmarked case is now named
accurately. call sites that were using `DEFAULT` to mean "something
happened" now say what they mean.


---


## toward GNUnet (ghr: v2)

a GNUnet-style server requires structured, machine-parseable logs.
the path from this interface to that is a backend change only:

    now:    log(Level, src, msg)
            → formatted string to stdout/stderr

    later:  log(Level, src, msg)
            → structured record { timestamp, level, src, msg }
            → routed to: stderr / file / syslog / wire

call sites never change. the interface is already structured —
`Level` and `src` are typed, discrete fields, not embedded in a
format string. extracting them into a record is a mechanical
transformation of the implementation, not a redesign.

the `constexpr const char*` source identifiers in `defines.hpp`
already anticipate this: they are the component identifiers a
distributed system would use to route and filter log streams.
Lukas' infrastructure is closer to correct than the enum suggests.


---


## the 1 property this design enforces

a call site specifies what it knows: significance and content.
it does not specify what it cannot know: how the system will handle
that significance (destination, format, filtering, persistence).

that separation — caller specifies semantics, system specifies
handling — is what makes the log infrastructure composable,
extensible, and honest.