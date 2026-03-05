# directory structure

## the layout

```
include/
    base/           — shared primitives (Fd, base.hpp, defines.hpp, ...)
    classes/        — concrete types (Listener, Connection, ...)
    interfaces/     — EpollHandler (abstract base class)
src/
    base/
    classes/
    interfaces/
```

three categories. the question is whether the categories are principled.


---


## base/

holds shared infrastructure with no domain logic: the Fd RAII wrapper,
defines.hpp (constants, macros), signal handling, logging. nothing in
base/ depends on anything in classes/ or interfaces/.

this is sound. base/ is the dependency floor — everything else can import
from it, nothing in it imports from above. a genuine layer.


---


## classes/

concrete types: Listener, Connection, WebServ, HttpParser. these have
full implementations, state, constructors. the word "classes" is a C++
term, not a domain concept — it names the mechanism, not the category.

this is acceptable but weak naming. "classes" means "things implemented
as C++ classes", which is everything. a stronger name would reflect what
the things are: handlers/, runtime/, domain/ — something that describes
the category's role, not its syntactic form.


---


## interfaces/

contains one entity: EpollHandler.

the word "interfaces" is imported from Java, where it names a specific
language construct: a type with method signatures and no implementation.
C++ has no such construct. what C++ has is abstract classes — classes with
at least one pure virtual method. abstract classes can also contain:

    non-virtual methods with implementations
    data members
    constructors

EpollHandler uses this. it contains `update_epoll_events()`, a non-virtual
method with a full implementation in EPollHandler.cpp. it is therefore not
an interface in the Java sense — it is an abstract base class with shared
behaviour.

the directory name imports a concept that does not exist in C++ and then
violates even that concept's definition by housing implementation code.


---


## engineering assessment

the separation of concerns is correct: abstract base class separated from
concrete implementations. that is sound.

the problems:

singular content in plural directory — `interfaces/` contains one entity.
either the name anticipates future content that never arrived, or the
abstraction was over-engineered for what was needed.

declaration/implementation split contradicts the directory's implied
contract — if `interfaces/` signals "declarations only", EPollHandler.cpp
living there is inconsistent.

the naming is Java-brained in a C++ codebase — it signals a mental model
where "interface" and "class" are distinct language-level categories. in
C++ they are not. the distinction is a matter of design intent, not
language enforcement.

other abstract base classes, if they arise, may not end up in
`interfaces/` — making the boundary ad hoc rather than principled.


---


## more C++-native alternatives

**keep abstract bases alongside concretes.**
the C++ convention is often to put Listener.hpp and EpollHandler.hpp in
the same headers directory, distinguishing them only by naming convention
(e.g. IEpollHandler, or simply by the presence of pure virtual methods).
the directory does not encode the distinction because the language does
not enforce it.

**name by role, not mechanism.**
if the intent is to house types that define contracts for the event loop
infrastructure, name that:

```
include/
    base/
    contracts/      — or: abstractions/, protocols/
    handlers/       — concrete event handlers
```

`contracts/` maps directly to what EpollHandler actually is: a set of
obligations that Listener and Connection must satisfy. this name is
language-agnostic, accurate, and forward-compatible with Agda/Haskell
thinking where "contract" and "typeclass/record type" are cognates.

**`abstract/`** is also used in practice and is at least honest about
the C++ mechanism, though it names the mechanism rather than the intent.


---


## forward: redesign from first principles

when webserv is redone — including as a GNUnet server — the directory
structure should be derived from the architecture, not inherited from Java
conventions. questions to reason through at that point:

what are the genuine layers?
    the dependency graph is the ground truth. a layer is a set of modules
    with a consistent dependency direction. name layers by what they are,
    not by C++ syntactic categories.

what is the right granularity for a "contract"?
    EpollHandler is one contract. are there others? (a CGI handler contract,
    a protocol handler contract for GNUnet transports?) if contracts
    proliferate, a contracts/ directory earns its place. if there is one,
    it probably belongs with its primary consumer.

where does the boundary between "infrastructure" and "domain" lie?
    base/ is infrastructure. Listener and Connection are domain. EpollHandler
    sits at the boundary — it is infrastructure in that the event loop uses
    it, but domain in that it encodes what a handler must do for this
    specific server. this boundary question sharpens when the server's
    protocol changes (HTTP → GNUnet).

does C++ remain the right language for the redo?
    GNUnet is a C codebase. a C++ server interfacing with GNUnet will have
    a FFI boundary. Rust would give memory safety and a richer type system
    for expressing contracts (traits are cleaner than abstract base classes
    for this pattern). this is worth reasoning through before the redo
    begins.

how would the contract look in Rust?

```rust
trait EpollHandler {
    fn fd(&self) -> RawFd;
    fn events(&self) -> u32;
    fn handle_event(&mut self, events: u32);
    fn should_close(&self) -> bool;
}
```
no vtable by default — trait objects (`dyn EpollHandler`) use dynamic
dispatch only when you ask for it. the directory question dissolves:
traits live alongside their primary implementors or in a dedicated
traits module, and the language makes the distinction explicit in the
type system rather than in directory naming conventions.