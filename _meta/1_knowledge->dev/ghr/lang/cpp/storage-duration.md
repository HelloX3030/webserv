# storage duration

## what it is

storage duration determines when memory for an object exists.
not *visibility* (who can name it), but *lifetime* (when it lives).

C++ has exactly 4 storage durations:

```
duration      allocated           deallocated
─────────────────────────────────────────────────────
automatic     block entry         block exit
static        program start       program end
dynamic       new                 delete
thread        thread start        thread end
```


## why these 4

C++ models execution as: program starts → main() runs → program ends.
within that, functions are called (stack frame born) and return (frame dies).

storage duration answers: what execution boundary governs this object's life?

automatic: scoped to a block (function call, control structure, bare {}).
static: scoped to the program. no smaller boundary available.
dynamic: scoped to explicit programmer action. no implicit boundary.
thread: scoped to thread execution. (C++11 onward.)

there is no "namespace-call" or "module-load" event in C++.
therefore namespace-scope variables cannot have "namespace duration."
program-scope is the only option. hence: static.


## automatic

```cpp
void foo()
{                       // block begins
    int x = 1;          // x allocated here
    {
        int y = 2;      // y allocated here
    }                   // y deallocated here
    // y no longer exists
}                       // x deallocated here
```

"block" means brace-delimited scope: function body, if/while/for body,
or standalone { }.

automatic objects are stack-allocated. LIFO order: last created, first
destroyed. destructor runs at block exit — this is RAII's foundation.


## static

```cpp
namespace Config {
    std::vector<Token> tokens;  // static duration
}

void bar()
{
    static int call_count = 0;  // static duration (function-local)
    ++call_count;
}
```

static duration arises from:
. namespace-scope declaration (outside any function)
. `static` keyword on local variable
. `static` keyword on class member

one instance exists for entire program. initialised before main()
(or on first use for function-local statics). destroyed after main().


## dynamic

```cpp
int* p = new int(42);   // allocated now
// ...
delete p;               // deallocated now
```

lifetime controlled entirely by programmer. no automatic cleanup.
use RAII wrappers (unique_ptr, shared_ptr) to bind dynamic lifetime
to automatic scope.


## thread

```cpp
thread_local int counter = 0;  // one per thread
```

each thread gets its own instance. born when thread starts, dies when
thread ends. useful for per-thread caches, thread-local error state.


---


## the critical insight

namespace-scope + mutable = shared state with program lifetime.

```cpp
namespace {
    std::vector<Token> tokens;  // static duration, internal linkage
}
```

even with internal linkage (invisible outside file), the storage is
static. one vector. all calls share it. not re-entrant. data races
possible if called from multiple threads.

instance members avoid this:

```cpp
class Parser {
    std::vector<Token> tokens_;  // duration depends on Parser instance
};
```

each Parser object owns its own vector. lifetime governed by how the
Parser itself is created (automatic, static, dynamic, or thread).


---


## other languages

Haskell: no mutable storage in base semantics. "global" bindings are
immutable values, not memory locations. no storage duration question
because no storage. mutable state requires explicit IORef with GC
managing lifetime via reachability, not scope.

Rust: same static/automatic/dynamic model as C++, but mutable statics
require `unsafe` or synchronisation primitives (Mutex). the type system
forces acknowledgment that static mutable state is a coordination problem.

Agda: purely functional, total. no mutable state, no storage duration
concept. all "data" is immutable terms. computation is term reduction,
not memory mutation.


---


## references

C++ standard [basic.stc]: storage duration definitions
C++ Core Guidelines: I.2, I.3 (avoid non-const global variables)