# /ghr/failure-response/

where to learn more here?

also in military context, esp. grand strategy,
also systems, cybernetics, operations research...




# _meta/1_knowledge->dev/ghr/observability/webserv_log-sys/debugging-logging-cpp.md

the idioms below are the stable building blocks.

"the building blocks" or simply some possible ones?


is conditional compilation ideal / best practice / done often?
give me context on it, its significance, usage!



# _meta/1_knowledge->dev/ghr/observability/webserv_log-sys/0_current-sys.md


understand:

```
what the 3 overloads provide

the 3 `log()` overloads are format variants, not semantic variants:

- `(title, msg, value, type)` — full form: msg + labelled value
- `(title, msg, type)` — message only; 
delegates to full form with `value = ""`
- `(title, i, msg, type)` — index form; converts `size_t` to string,
  delegates to full form. default type: `LIST`

all 3 route to the 4-argument form. the overloads exist to spare
call sites from manual `std::to_string(i)` and empty-string arguments.
```








(from 20260305-4_logging-debugging_observability)
knowledge creation:

```
source identifiers — what they are and why
cpp
constexpr const char* WEB_SERV    = "WebServ";
constexpr const char* CONNECTION  = "Connection";
constexpr const char* HTTP_PARSER = "HttpParser";
These are named string constants — not macros. The distinction matters: #define WEB_SERV "WebServ" is textual substitution by the preprocessor before compilation. constexpr const char* is a typed, scoped, compile-time constant processed by the compiler itself.
Why not write "WebServ" inline at every call site?

a string literal has no name. if you write "WebServ" in 30 places and later rename the component, you change 30 sites and risk missing one. with a named constant: change 1 line.
a typo in a string literal ("WebSrv") is silent — no compiler error, broken log output. a typo in the constant name (WEB_SRV) is a compile error.
reading log::log(CONNECTION, "bytes read") at a call site is semantically clear. reading log::log("Connection", "bytes read") requires the reader to trust the string is correct.

Why constexpr const char* and not std::string?
std::string is heap-allocated at runtime. const char* to a string literal is a pointer into the read-only data segment — zero runtime cost, exists for the lifetime of the program. for values that are fixed at compile time and used in a hot path (every log call), const char* is the correct choice.
This is a narrow application of a broad principle: give names to the vocabulary of your system. the component names are part of the system's semantic vocabulary. name them once, in one place.
```








# general

`>` symbol in directory name
made git rm impossible

why?
what are "special symbols" which can be interpreted differently than normal letters in POSIX-compliant systems? anything outside of an ascii range?

need to learn etiquette for file systems: naming of dirs, files,
both in terms of structure-content AND style (what info to include, what file names should represent)...how much information to include, where.






# version control

git branches

ideal:?
branch per feature
merge together

but since I'm working now on different features concurrently,
want to keep stable state, minimise merge complexity - okay to work on 1 main branch for all my features? esp when merge frequency around once per week, within this time, need to bring a few features forward











# 20260306-3_build_makefile


20260305-1_make-build
20260306-3_build_makefile
20260306-4_build-make




then some specific knowledge aspects:

flags, options...
acronymes

take apart, profile each in own knowledge node





2_declarative-functional-paradigm.md

```
the engine resolves a goal graph
```

real term?





user-facing doc/ to include info on how to build with this Makefile.
types of builds, their options...





# doc/

for eval,

doc/ user facing
remove _meta?




Linux only

build system
    summary from _meta





# logs

20260306-2_observability_failure-management

see last section:
```
The telos of log formatting: make events findable and interpretable at any future time, by any consumer — human or tool — without prior knowledge of context.
```

structured logging, use of Unix tools.
is this info in the docs on failure-response/ or observability/ ?