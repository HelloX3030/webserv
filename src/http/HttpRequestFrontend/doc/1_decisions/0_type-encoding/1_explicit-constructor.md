# explicit constructor

## context
```cpp
explicit HttpRequestFrontend(size_t max_body_size);
```

single-argument constructor taking a configuration value.

## problem

without `explicit`, implicit conversion is permitted:
```cpp
HttpRequestFrontend frontend = 1024;  // compiles
```

a bare integer silently becomes a stateful parser.
the integer carries no semantic signal — is 1024 a body limit?
a buffer size? an fd? the code does not say.

danger scenario:
```cpp
void attach_to_connection(HttpRequestFrontend& frontend);

// in Connection code
attach_to_connection(config.max_body);  // typo: passed size_t
                                         // without explicit: compiles
                                         // creates temporary, discards it
                                         // silent bug
```

## decision

mark `explicit`.

the test: does `Frontend f = 1024;` read as meaningful English?
"a frontend *is* 1024" — nonsense. therefore `explicit`.

single-argument constructors that represent *configuration* should be explicit.
constructors that represent *conversion* (e.g., `std::string(const char*)`) may be implicit.

## reference

C++ Core Guidelines C.46: by default, declare single-argument
constructors explicit.
