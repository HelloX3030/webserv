# header files

## what they are

a header file is an interface contract.

it declares what a translation unit offers: types, functions, constants.
other translation units include the header to use those declarations.

the header says *what exists*. the source file says *how it works*.


## why they exist

C++ uses separate compilation. each `.cpp` file compiles independently
into an object file. the linker combines object files into an executable.

problem: if `a.cpp` calls a function defined in `b.cpp`, how does the
compiler know the function exists and what its signature is?

solution: `b.hpp` declares the function. `a.cpp` includes `b.hpp`.
the compiler sees the declaration, trusts it, generates a call.
the linker resolves the call to the actual definition in `b.o`.

```
b.hpp          a.cpp              b.cpp
──────         ──────             ──────
void foo();    #include "b.hpp"   #include "b.hpp"
               foo();             void foo() { ... }
                  │                     │
                  v                     v
               a.o                   b.o
               (call to foo)        (definition of foo)
                  │                     │
                  └──────┬──────────────┘
                         v
                     executable
                     (call resolved)
```

the header is the bridge between separate compilation units.


## the inclusion model

`#include "header.hpp"` is textual substitution. the preprocessor
replaces the directive with the entire contents of the file.

consequence: everything in the header is copied into every file that
includes it. if 10 files include `base.hpp`, the compiler processes
`base.hpp` contents 10 times.


## the recompilation problem

the compiler tracks dependencies. if any included file changes, the
translation unit recompiles.

```
base.hpp ──────┬──────> a.cpp ──> a.o
               │
               ├──────> b.cpp ──> b.o
               │
               └──────> c.cpp ──> c.o
```

change `base.hpp` → recompile a.cpp, b.cpp, c.cpp.

if `base.hpp` contains implementation details that only `a.cpp` uses,
b.cpp and c.cpp still recompile. wasted work.

this is why headers should be minimal.


## correct discipline

### rule 1: header includes only what its interface requires

the header includes headers for types that appear in:
- function signatures (parameters, return types)
- base classes
- non-pointer/reference members

it does not include headers used only in the implementation.

```cpp
// parser.hpp
#include "config.hpp"    // ServerConfig in return type
#include <string>        // std::string in parameter
#include <vector>        // std::vector in return type

// does NOT include <fstream> — used only in .cpp
```

### rule 2: source includes exactly what it uses

each `.cpp` includes headers for every symbol it directly uses.

```cpp
// parser.cpp
#include "parser.hpp"
#include <fstream>       // std::ifstream
#include <stdexcept>     // std::runtime_error
#include <iterator>      // std::istreambuf_iterator
```

the include list documents dependencies. reading it tells you what
the implementation touches.


## forward declarations

when a header only needs to know a type exists (not its definition),
a forward declaration suffices:

```cpp
class ServerConfig;  // forward declaration

class Parser {
    ServerConfig* config_;  // pointer — incomplete type OK
    ServerConfig& ref_;     // reference — incomplete type OK
};
```

vs:

```cpp
#include "config.hpp"  // full definition — heavier

class Parser {
    ServerConfig config_;  // value — requires complete type
};
```

forward declaration sufficient when:
- type appears only as pointer or reference
- no construction, destruction, or member access in header

benefit: breaks include chains, reduces recompilation.


## include guards

headers need protection against multiple inclusion:

```cpp
// parser.hpp
#ifndef PARSER_HPP
#define PARSER_HPP

// ... contents ...

#endif
```

or:

```cpp
#pragma once  // non-standard but universally supported
```

without guards, if `a.hpp` includes `b.hpp` and `c.hpp`, and both
`b.hpp` and `c.hpp` include `d.hpp`, then `d.hpp` contents appear
twice — redefinition errors.


## the bundled-header anti-pattern

```cpp
// base.hpp — pulls in everything
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
// ... 20 more headers ...
```

appears convenient. creates 3 problems:

recompilation cascade: any change to base.hpp recompiles all includers.

dependency leakage: files gain access to symbols they don't declare
dependency on. invisible, unmaintainable coupling.

false contract: the header claims "to use me, pay for all of this."
every includer pays full cost regardless of what it uses.


---


## other languages

Haskell: no header files. each module is self-contained. `import`
brings names into scope. the compiler reads the imported module
directly. separate compilation via interface files (`.hi`) generated
automatically.

Rust: no header files. `mod` and `use` declare module structure.
the compiler sees all source. visibility controlled by `pub`.
incremental compilation built into the toolchain.

Agda: no header files. modules with explicit imports/exports.
the type checker processes dependencies as needed.

the header/source split is a C legacy. languages designed later
handle separate compilation without manual interface files.


---


## summary

header = interface contract. source = implementation.

include in header only what interface requires.
include in source exactly what implementation uses.

minimise headers to minimise recompilation and coupling.

forward-declare when possible. include when necessary.


---


## references

C++ Core Guidelines: SF.1 (use .h suffix for headers), SF.2 (header
should be self-contained), SF.8 (use include guards)

Lakos, J. (1996). "Large-Scale C++ Software Design." — physical design,
levelisation, include dependency management.