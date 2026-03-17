# state and concurrency

## the problem

mutable state + multiple threads = coordination problem.

if 2 threads read and write the same memory without synchronisation,
behaviour is undefined. this is a data race.

the question becomes: who owns this state? how many can access it
simultaneously?


## state ownership patterns

### pattern 1: instance-owned state

```cpp
class Parser {
    std::vector<Token> tokens_;
    size_t pos_;
public:
    Config parse(const std::string& path);
};
```

each Parser object owns its own tokens_ and pos_.

```cpp
// thread 1
Parser p1;
p1.parse("a.conf");  // uses p1.tokens_, p1.pos_

// thread 2
Parser p2;
p2.parse("b.conf");  // uses p2.tokens_, p2.pos_
```

different objects, different state, no sharing, no coordination needed.

properties:
- re-entrant: yes
- thread-safe: yes (if instances are not shared)
- lifetime: tied to object lifetime


### pattern 2: file-static state

```cpp
// parser.cpp
namespace {
    std::vector<Token> tokens;  // static duration
    size_t pos;                 // static duration
}

Config parse(const std::string& path) {
    tokens.clear();
    tokenise(path);
    pos = 0;
    // ... uses tokens and pos ...
}
```

one tokens vector, one pos. all calls share them.

```cpp
// thread 1
parse("a.conf");  // modifies the ONE tokens, the ONE pos

// thread 2 (simultaneously)
parse("b.conf");  // modifies the SAME tokens, the SAME pos
```

thread 1 is reading tokens while thread 2 is clearing it.
undefined behaviour.

properties:
- re-entrant: no
- thread-safe: no
- lifetime: program duration


### pattern 3: explicitly synchronised state

```cpp
namespace {
    std::mutex mtx;
    std::vector<Token> tokens;
    size_t pos;
}

Config parse(const std::string& path) {
    std::lock_guard<std::mutex> lock(mtx);  // acquire
    tokens.clear();
    tokenise(path);
    pos = 0;
    // ...
}  // release
```

mutex ensures only one thread executes the critical section.

properties:
- re-entrant: no (same thread would deadlock on recursive call)
- thread-safe: yes
- lifetime: program duration
- cost: serialisation — threads wait for each other


### pattern 4: thread-local state

```cpp
thread_local std::vector<Token> tokens;
thread_local size_t pos;

Config parse(const std::string& path) {
    tokens.clear();
    // ...
}
```

each thread gets its own copy. no sharing between threads.

properties:
- re-entrant: no (within same thread, state is shared)
- thread-safe: yes (between threads)
- lifetime: thread duration


---


## the design space

```
                    sharing
                    │
         none ◄─────┼─────► all
                    │
    instance-owned  │  file-static
    thread-local    │  synchronised global
```

less sharing = less coordination = simpler reasoning = safer.

more sharing = more coordination = more complexity = more bugs.


## re-entrancy

a function is re-entrant if it can be interrupted mid-execution
and called again (from the same or another thread) without corrupting
state.

requirements:
- no static/global mutable state, OR
- all static/global state is protected, OR
- all static/global state is thread-local

instance-owned state is inherently re-entrant (for different instances).

file-static mutable state is inherently non-re-entrant unless protected.


## when file-static is acceptable

conditions:
1. single-threaded context (no other threads exist)
2. called exactly once (startup initialisation)
3. state is read-only after initialisation

example: config parsing at program startup.

```cpp
int main() {
    // no other threads exist yet
    auto config = parse("server.conf");  // file-static OK here
    
    // now spawn threads, but parse() never called again
    start_server(config);
}
```

the assumptions are:
- called before threads exist
- never called again

if these assumptions break, the code breaks.


## making assumptions explicit

file-static with hidden assumptions:

```cpp
Config parse(const std::string& path);  // signature reveals nothing
```

caller cannot know this is not thread-safe.

instance-based with explicit semantics:

```cpp
class Parser {
    Config parse(const std::string& path);
};
```

caller knows: create instance, use instance, instance owns state.
thread safety follows from not sharing instances.


---


## other languages

Haskell: no mutable state by default. concurrency via immutable data
and explicit concurrency primitives (MVar, STM). data races impossible
on pure values — nothing to race on.

Rust: ownership system prevents data races at compile time. mutable
references are exclusive — only one &mut at a time. shared references
are immutable. Arc<Mutex<T>> for shared mutable state, explicit in type.

```rust
// compiler rejects:
let mut v = vec![1, 2, 3];
let r1 = &mut v;
let r2 = &mut v;  // error: cannot borrow v as mutable twice
```

Agda: purely functional, total. no mutable state. no concurrency
primitives in core language. concurrent Agda exists as research
extension with session types.


---


## summary

instance-owned state: safest. each owner manages their own. no
coordination. re-entrant. thread-safe by isolation.

file-static state: dangerous. shared by all. requires synchronisation
or strict single-threaded/single-call discipline.

the choice is architectural. instance-based makes correctness structural.
file-static makes correctness conventional (depends on programmer
discipline).

prefer structural correctness. conventions fail under maintenance.


---


## references

C++ Core Guidelines: CP.2 (avoid data races), CP.3 (minimise explicit
sharing), CP.31 (pass small amounts of data by value)

Sutter, H. "Effective Concurrency" series.

Williams, A. (2019). "C++ Concurrency in Action." — comprehensive
treatment of C++ threading model.