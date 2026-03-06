# propagation

a failure detected deep in a call chain must reach the site equipped
to handle it. propagation is the mechanism by which this happens.

in C++: exceptions. a `throw` unwinds the call stack, frame by frame,
until a matching `catch` is found. intermediate layers require no
knowledge of the failure — they are simply unwound.


---


## the exception hierarchy

```
std::exception
    ├── std::logic_error      — violated precondition; programming error.
    │                           detectable by inspection. should not occur.
    └── std::runtime_error    — contingent on runtime state.
                                cannot be predicted or prevented by the
                                program. e.g. file not found, malformed
                                input, socket error.
```

`std::exception` defines 1 method: `what()`, which returns the
message string. it is the root type — a `catch (const std::exception&)`
catches any standard exception and recovers the message via `what()`.

the branch distinction is semantic, not mechanical. both propagate
identically. the choice of type declares the nature of the failure:
is this a bug in the program (`logic_error`) or a condition of the
runtime environment (`runtime_error`)?

config parse failures, I/O failures, and protocol violations are
`runtime_error`: they depend on operator input and environment, not
on program correctness.


---


## throw and catch mechanics

```cpp
// detection site — deep in call chain
throw std::runtime_error("[config] line 12: expected ';'");

// handling site — main.cpp
catch (const std::exception& e) {
    log::log(WEB_SERV, e.what(), log::LogType::ERROR);
    WebServ::quit();
    return 1;
}
```

`throw` constructs the exception object and transfers control.
stack unwinding begins: destructors of all local objects in each
frame are called as the stack is walked upward. RAII resource
cleanup happens automatically here — this is not incidental but
the designed interaction between exceptions and destructors.

`catch (const std::exception& e)` matches any exception in the
`std::exception` hierarchy. catching by const reference avoids
slicing: the concrete type (`runtime_error`) is preserved, and
`what()` returns the correct message.

if no matching `catch` is found anywhere in the call stack,
`std::terminate` is called. the program ends without cleanup.
this is why the top-level catch in `main` is necessary and must
not be omitted.


---


## why not error codes or out-parameters

2 alternatives exist for propagation:

return values:
```cpp
// every intermediate function must check and forward
int parse(...) {
    int err = tokenise(...);
    if (err) return err;
    err = parse_blocks(...);
    if (err) return err;
    return 0;
}
```

exceptions:
```cpp
// intermediate functions are transparent to failure
void parse(...) {
    tokenise(...);    // throws on failure
    parse_blocks(...); // throws on failure
}
```

with return values: every intermediate layer must explicitly check
and forward the error. the failure path is as verbose as the success
path. one forgotten check silently swallows the failure.

with exceptions: intermediate layers are transparent. the throw site
and the catch site are coupled directly. the call chain between them
requires no changes.

the cost: non-local control flow. reading a function, you cannot
see which calls may throw. this is the genuine trade-off — exceptions
buy propagation simplicity at the cost of visible control flow.

for deep call chains with fatal failures, as in config parsing,
the trade-off resolves clearly in favour of exceptions.


---


## the contract in webserv

throw at the detection site, with a precise message.
catch once in `main`.
no intermediate catches. no rethrowing. no error codes.

the message string is the sole carrier of diagnostic information.
`what()` is the only interface between the throw site and the
operator. therefore the message must be self-sufficient at the
point of construction — where context is richest.

by the time the exception reaches `main`, the call stack is gone.
line numbers, field values, and local context exist only if they
were embedded in the message at the throw site.