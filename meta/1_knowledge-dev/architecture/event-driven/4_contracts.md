# contracts

## what a contract is

a contract in a type system is a named set of obligations: if you claim to
be this type, you must provide these capabilities. it separates what
something can do from how it does it.

the caller binds to the contract. the implementor binds to the contract.
they do not bind to each other. this indirection is what permits
substitution — swapping one implementor for another without changing the
caller.

in webserv, the contract is `EpollHandler`. the caller is `WebServ::run()`.
the implementors are `Listener` and `Connection`.


---


## EpollHandler: each method and its necessity

```cpp
class EpollHandler {
    virtual int      get_fd()                  const = 0;
    virtual uint32_t get_events()              const = 0;
    virtual void     handle_event(uint32_t)          = 0;
    virtual bool     should_close()            const = 0;
    virtual std::string to_string()            const = 0;

    void update_epoll_events();   // non-virtual, shared implementation
    virtual ~EpollHandler();
};
```

each method is there because the dispatcher or the infrastructure requires
it. there is no surplus.

**`get_fd()`** — the dispatcher must be able to recover the fd from a
handler to call `remove_epoll_handler(fd)`. the fd is also needed by
`update_epoll_events()` to call `epoll_ctl(EPOLL_CTL_MOD)`. required.

**`get_events()`** — used at registration time (`add_epoll_handler`) and
at update time (`update_epoll_events`). the handler declares which events
it is interested in. Listener always returns EPOLLIN. Connection returns
EPOLLIN | EPOLLOUT depending on state. the dispatcher does not decide this
— each handler knows its own interest.

**`handle_event(uint32_t events)`** — the core dispatch target. the reason
the interface exists. required.

**`should_close()`** — the dispatcher checks this after handle_event().
rather than the dispatcher inspecting internal state (which it cannot do,
knowing only EpollHandler*), the handler exposes a boolean predicate. this
keeps the state machine private to the handler. required.

**`to_string()`** — debug logging. not required for correctness. present
because the debug path in run() calls it:

```cpp
log::log(WEB_SERV, handler->get_fd(), handler->to_string());
```

if debug mode were excluded from consideration, this method would not be
in the contract.

**`update_epoll_events()`** — non-virtual. it is implemented once in
EpollHandler.cpp, using `get_fd()` and `get_events()` (which are virtual).
concrete handlers call it when their interest mask changes. it is shared
infrastructure, not a per-handler behaviour — so it does not belong in the
virtual interface. this is correct design: the contract specifies behaviour,
the base class provides shared utilities.

**`virtual ~EpollHandler()`** — any base class whose derived instances will
be deleted through a base pointer must have a virtual destructor. if the
destructor is non-virtual, `delete handler` (where handler is EpollHandler*)
calls only EpollHandler's destructor, not Listener's or Connection's — a
resource leak. making it virtual ensures the correct destructor chain
executes. this is a C++ mechanical requirement, not a domain obligation.


---


## C++ abstract class

C++ has no dedicated `interface` keyword. an abstract class — one with at
least one pure virtual method — serves the same role. EpollHandler is an
abstract class.

the distinction between abstract class and concrete class is enforced by
the compiler: instantiating an abstract class is a compile error.

an abstract class in C++ can contain:
    pure virtual methods (the contract obligations)
    non-virtual methods with implementations (shared utilities)
    data members
    constructors

this is more permissive than a Java interface (historically) or a Haskell
typeclass. whether this permissiveness is good design depends on use.
EpollHandler uses it correctly: the non-virtual `update_epoll_events()`
is genuinely shared behaviour, not a contract obligation.


---


## Java interface

Java separates the interface keyword from class inheritance explicitly.

```java
interface EpollHandler {
    int getFd();
    int getEvents();
    void handleEvent(int events);
    boolean shouldClose();
    String toString();
}

class Listener implements EpollHandler { ... }
class Connection implements EpollHandler { ... }
```

a Java interface (prior to Java 8) contains only method signatures — no
implementation, no data. it is a pure contract. from Java 8, `default`
methods allow implementations in interfaces, moving Java closer to C++'s
abstract class.

Java permits a class to implement multiple interfaces but inherit from only
one class. this resolves the diamond problem for contracts while retaining
single-inheritance for implementation.

the webserv EpollHandler has no data members, and its only non-virtual
method (`update_epoll_events`) could be a default method in Java. the
design maps cleanly.


---


## Haskell typeclass

a typeclass is a contract over types, not over values.

```haskell
class EpollHandler a where
    getFd       :: a -> Int
    getEvents   :: a -> Word32
    handleEvent :: a -> Word32 -> IO a   -- returns updated handler (immutable data)
    shouldClose :: a -> Bool
    toString    :: a -> String
```

where C++ and Java express contracts over objects (runtime values with
identity), Haskell expresses contracts over types. the difference:

    C++/Java: "this object can do X" — resolved at runtime via vtable/dispatch
    Haskell:  "values of type T can do X" — resolved at compile time via
              dictionary passing (the compiler generates a typeclass dictionary,
              analogous to a vtable, but the selection happens at compile time
              when the type is known)

in Haskell, `handleEvent` returns an updated handler value rather than
mutating in place. the state machine that Connection implements via mutable
fields would instead be expressed as a pure state transition:

```haskell
handleEvent :: Connection -> Word32 -> IO Connection
handleEvent conn events = ...   -- returns new Connection with updated state
```

no mutable state, no pointers. the event loop would be:

```haskell
runLoop :: [EpollHandler a => a] -> IO ()
```

though in practice Haskell would use existential types or a sum type to
heterogeneously store different handler types — this is the point at which
the direct structural analogy breaks down, because Haskell's type system
requires different machinery to express "a collection of values of different
types that all satisfy the same constraint."

the core insight: typeclass and virtual dispatch solve the same problem —
open-ended polymorphism — but in radically different positions in the
compile-time/runtime axis.


---


## Agda

in Agda, interfaces become record types, and typeclass-like constraints
become record parameters or module parameters.

```agda
record EpollHandler (A : Set) : Set where
    field
        getFd       : A → ℕ
        getEvents   : A → ℕ
        handleEvent : A → ℕ → IO A
        shouldClose : A → Bool
```

the difference from Haskell: in Agda, you pass the record (the "dictionary")
explicitly as a value. there is no implicit resolution. this makes the
machinery visible and formally verifiable.

the deeper point for Agda: contracts become propositions, and implementation
correctness becomes proof. you can state not only "Connection implements
EpollHandler" but "Connection implements EpollHandler and its handle_event
satisfies invariant P" — and prove it within the type system. this is the
transition from programming to verified programming.


---


## the structural question: why not a free function + switch?

one might ask: why have an interface at all? the dispatcher could inspect
a type tag:

```cpp
enum HandlerType { LISTENER, CONNECTION };

struct EpollHandler {
    HandlerType type;
    int fd;
};

void dispatch(EpollHandler *h, uint32_t events) {
    switch (h->type) {
        case LISTENER:   handle_listener(h, events); break;
        case CONNECTION: handle_connection(h, events); break;
    }
}
```

this works for two types. when a third type is added (CgiPipe), the switch
must be modified. the dispatcher couples to every handler type. the
interface design avoids this: the dispatcher is written once and never
modified when new handler types are added.

the switch formulation localises dispatch in one place but couples that
place to all types. the virtual dispatch formulation distributes dispatch
across the type hierarchy but decouples the dispatcher. the right choice
depends on whether new handler types are anticipated. in webserv, they are
(CGI requires pipe handlers) — virtual dispatch is correct.