# webserv log system — current implementation

Lukas' log module. files: `include/base/log.hpp`, `src/base/log.cpp`.


---


## structure

```cpp
enum class LogType { DEFAULT, ERROR, LIST };

void log(string title, string msg, string value, LogType);
void log(string title, string msg, LogType);
void log(string title, size_t i, string msg, LogType);
```

source identifiers (titles) are `constexpr const char*` strings,
defined in `include/base/defines.hpp`:

    WEB_SERV, LISTENER, CONNECTION, HTTP_PARSER, DISPLAY, ...

passed as the `title` argument at every call site.


---


## what the 3 overloads provide

the 3 `log()` overloads are format variants, not semantic variants:

- `(title, msg, value, type)` — full form: msg + labelled value
- `(title, msg, type)` — message only; delegates to full form
  with `value = ""`
- `(title, i, msg, type)` — index form; converts `size_t` to string,
  delegates to full form. default type: `LIST`

all 3 route to the 4-argument form. the overloads exist to spare
call sites from manual `std::to_string(i)` and empty-string arguments.


---


## what the 3 LogType values actually do

reading `log.cpp` against the enum:

    DEFAULT  →  stdout,  format: [title] msg: value
    ERROR    →  stderr,  format: [title] ERROR: msg: value
    LIST     →  stdout,  format: [title] value: msg  (args swapped)

2 distinct effects are present: routing (ERROR → stderr; DEFAULT and
LIST → stdout) and argument order (DEFAULT → msg first; LIST →
value first).

`LogType` conflates these. `ERROR` encodes a routing and prefix
decision. `DEFAULT` vs `LIST` encodes a formatting preference.
these are orthogonal axes collapsed into 1 enum — a category error.

`LIST` is not a severity or a kind of event. it is a call-site
presentational preference. naming it as a `LogType` peer of `ERROR`
misrepresents its nature.


---


## severity vocabulary

the system has no severity gradient.

`ERROR` signals failure and routes to stderr. `DEFAULT` and `LIST`
are equivalent in severity — both signal "something happened" and
route to stdout.

absent: any distinction between normal operational events (connection
accepted, request parsed), anomalies that are handled (malformed
header, timeout), and failures (socket error, bad config).

in a debug session, all non-error output arrives at the same level
of urgency, with no mechanism to suppress or filter by significance.


---


## conditional compilation

debug-only assertions are gated with `#ifdef DEBUG`:

```cpp
#ifdef DEBUG
    if (title.length() > log_title_width - 2)
        std::cout << "LOG TITLE TO LONG!" << std::endl;
#endif
```

the broader debug mode — `log::log` calls throughout the event loop,
`to_string` on handlers — is also gated this way at call sites
(`main.cpp`, `WebServ_init.cpp`).

`DEBUG` is defined at compile time via `-DDEBUG`. debug and release
are genuinely distinct binaries. the diagnostic code has zero cost
in release — it is absent from the translation unit.


---


## `to_string(LogType)`

declared as a free function outside the `log` namespace:

```cpp
std::string to_string(log::LogType type);
```

used in exactly 1 place: the `else` branch in `log.cpp` that throws
when an unknown `LogType` is encountered.

the call site is inside `namespace log`, so unqualified lookup would
find the function there regardless of placement. the reason it sits
at global scope is convention: all `to_string` overloads in this
codebase (`ListenAddress`, `Location`, `ServerConfig`) are free
functions at global scope, following the `std::to_string` pattern.
placement is consistent, not driven by lookup mechanics.


---


## call site pattern

```cpp
log::log(CONNECTION, "bytes read", n_str, log::LogType::DEFAULT);
log::log(HTTP_PARSER, i, "header parsed");   // LIST default
log::log(WEB_SERV, e.what(), log::LogType::ERROR);
```

`LogType::DEFAULT` is the default argument — omitting the type
argument implies DEFAULT. DEFAULT is the unmarked case; ERROR and
LIST are the marked cases.


---


## assessment

functional for a 42 project. `LogType` conflates routing, formatting,
and (implicitly) severity into 1 enum — the values are not members
of the same noetic category. separately: no severity gradient exists;
all non-error events are undifferentiated, with no mechanism to
filter output by significance beyond the binary DEBUG/release split.

tolerable at current scale. both become liabilities as output volume
grows.