# modules vs types

before any language mechanism is chosen:
does this thing model an entity or a process?


---


## entity vs process

an entity has persistent identity, can be instantiated, has state that
evolves across its lifetime. it is a noun: a connection, a server, a handle.

a process transforms input into output. transient intermediate state may
exist, but nothing persists beyond a single invocation. it cannot be
instantiated — "running the parse pipeline" twice produces 2 results,
not 2 parsers. it is a verb: parsing, tokenising, validating.


---


## c++ constructs

free functions — standalone transformations, no associated state.

struct/class — a type. defines a species of entity: instantiation,
lifetime, identity, value semantics.

namespace — named scope, organisational only. no instances, no lifecycle.

anonymous namespace — translation-unit-local. names declared here are
invisible to other translation units: the correct home for implementation
details that must not leak.


"class or namespace?" maps directly to "entity or process?".


---


## a misuse pattern: class as module

symptoms: never instantiated more than once simultaneously, no meaningful
question of "how many instances?", public interface is 1 function, member
variables are transient — they exist only during a call, the class is
never stored, passed, or returned.

cost: a class in a header exposes internal types and member variables —
even if private, they appear in the declaration and propagate recompilation
on change. a namespace with implementation in an anonymous namespace
exposes only the public interface.


---


## transient state and tramp data

mutually recursive functions sharing intermediate state — a cursor, a
partially-built result — face a choice. passing state as a parameter
through every level makes it tramp data: a passenger carried not because
the immediate function uses it, but because something below does.
tramp data falsely implies every function depends on every parameter.


3 solutions:

class — eliminates tramp parameters via implicit `this`. correct locally,
but overclaims: creates a type where no type is needed.

local state struct in anonymous namespace — a struct whose sole purpose
is carrying shared state through a pipeline. not a domain type. lives
in the .cpp, invisible to all includers. instantiated once per invocation,
destroyed on return. transience is structurally expressed.

explicit passing — correct when call depth is shallow and shared state
is small. tramp data is a problem of depth and breadth, not of principle.


for a recursive descent parser — 6+ call levels, 2 shared cursors —
the local state struct is correct.


---


## canonical form: stateless transformation module

header:

```cpp
namespace ModuleName {
    OutputType transform(InputType const&);
}
```

source:

```cpp
namespace {
    struct State {
        // transient state shared across the pipeline
        OutputType run(InputType const&);
    };
}

namespace ModuleName {
    OutputType transform(InputType const& input) {
        return State{}.run(input);
    }
}
```

the header carries zero implementation detail. internal types are
invisible to all includers. State's lifetime is exactly 1 call.


---


## applied: ConfigFrontend. analysis of initial implementation

input: filepath. output: vector<ServerConfig>. transient state: token
vector, cursor position. persistent state: none. never stored, passed,
or returned.

implemented as a class. the local argument — eliminate tramp data across
a 6-level recursive descent — was valid, but the prior question was
never asked.

correct form: ConfigFrontend.hpp exposes the namespace and parse().
ConfigFrontend.cpp contains the anonymous-namespace Parser struct and
its methods. the split .cpp files (_2b_, _2c_, ...) collapse into 1
file — they required the class declaration in the header to share a
type across translation units. with Parser in an anonymous namespace,
that coupling disappears. pipeline stages become sections within 1 file,
in call order.


---


## language perspectives

Haskell — a parsing pipeline is a function or a composition of functions.
shared cursor state is threaded via the State monad, or abstracted by a
parser combinator library (Parsec, Megaparsec). no class is reached for.

Rust — a free function or impl block on a local struct. the struct is
not exported; it appears only in the .rs file. the module system (mod,
pub) maps directly to the namespace/anonymous-namespace distinction.

Agda — a pipeline is a function between types. intermediate state is
made explicit in the type signature. "class or module?" does not arise:
there are no classes, only types and functions.


---


## references

Lakos, J. Large-Scale C++ Software Design. ch. 1-2.
    physical design and the consequences of header dependencies.

Stroustrup, B. The Design and Evolution of C++. ch. 3.
    the class as type, not as module — the original intent.

Sutter, H. Exceptional C++. item 39.
    namespaces and the interface principle.
