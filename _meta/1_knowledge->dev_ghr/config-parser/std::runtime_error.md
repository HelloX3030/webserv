# `std::runtime_error`

defined in `<stdexcept>`.

inheritance:
```
std::exception
    └── std::runtime_error
```

`std::exception` is the root of the C++ standard exception hierarchy.
`what()` is its polymorphic accessor — returns the message string.

consequence: a `catch (const std::exception& e)` block catches any
standard exception, including `std::runtime_error`, and recovers
the message via `e.what()`. this is why a single catch site in
`main` can handle errors thrown anywhere in the program without
knowing the concrete exception type.

---

## semantic category

the standard library divides exceptions into 2 branches:
```
std::exception
    ├── std::logic_error     — violated precondition, programming error.
    │                          detectable by inspection. should not occur.
    └── std::runtime_error   — contingent on runtime state.
                               cannot be predicted or prevented by the program.
                               e.g. file not found, malformed input, invalid config.
```

`std::runtime_error` is correct for config parser failures because
the error condition (malformed config, absent file) is not a bug in
the program — it is a fact about the environment at runtime.
`std::logic_error` would be semantically wrong: it signals that the
programmer made an error, not the operator.

---

## usage pattern in this program

throw at the site of detected failure, with a precise message:
```cpp
throw std::runtime_error("[config] line " + std::to_string(line)
    + ": expected ';'");
```

catch once in `main`, log, exit:
```cpp
catch (const std::exception& e) {
    log::log(WEB_SERV, e.what(), log::LogType::ERROR);
    return 1;
}
```

no error codes. no per-layer rethrow. message string is the sole
carrier of diagnostic information.