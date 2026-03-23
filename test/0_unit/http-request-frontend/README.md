# HttpRequestFrontend — unit tests


## purpose

exercises `HttpRequestFrontend::advance()` in isolation.
bytes in, `ParseResult` out. no I/O, no Connection, no epoll.


---


## harness

hand-rolled, C++20. 2 macros, the rest is functions and templates.

`TEST(name)` — defines and registers a test function.
automatic registration via static initialisation before `main`.

`ASSERT_TRUE(cond)` — thin wrapper: stringifies the expression
(preprocessor), delegates to a function using `std::source_location`
for file and line. the stringification is irreducible without
the preprocessor.

`assert_eq(expected, actual)` — template function.
`std::source_location` captures caller site.
requires `operator==` and `operator<<` on `T`.

failure semantics: throw on first failed assertion within a test.
the runner catches per test — 1 broken test does not kill the suite.


---


## structure
```
test_harness.hpp     registration, assertions, runner
test_helpers.hpp     operator<< for ParseStatus, advance helpers
main.cpp             entry point: calls run_all_tests()
```

test TUs are separate files, linked in. adding a test:
write a `TEST(name)` in any linked TU. no edits elsewhere.


---


## build
```
make          build release
make run      build + run
make debug    build with -DDEBUG -g -O0
make leaks    build for valgrind
make leaksrun build + run under valgrind
```


---


## test scope

request-line: valid methods, version handling, SP structure,
error codes 400/501/505.

headers: field parsing, OWS trimming, Host enforcement,
Content-Length/chunked branching, 413 detection.

body: Content-Length consumption, chunked decoding,
boundary splits, pipelining across `reset()`.

cross-cutting: byte-at-a-time delivery, all error codes.
