# c++ includes, headers, and dependency discipline

last updated [20260221]

---

## the problem: what does `#include` actually do?

`#include` is a preprocessor directive.
it performs textual substitution: the contents of the named file are
inserted verbatim at that point in the translation unit before
compilation begins.

consequence: every symbol, type, macro, and declaration in the
included file becomes visible to the including file.
every file that included file itself includes also becomes visible.
this propagates transitively and without limit.

this is the root of every include-discipline problem.
`#include` is not a module import. it is copy-paste at compile time.

---

## translation units

the C++ build model:

each `.cpp` file is compiled independently into an object file.
this independent unit of compilation is called a translation unit.
the linker then combines object files into the final binary.

the compiler sees exactly 1 translation unit at a time.
it does not know what other `.cpp` files exist.
everything a `.cpp` needs must be reachable from its own includes.

---

## the role of a header

a header declares an interface.
it answers: what names exist, what types are they, what are their
signatures?

it does not define implementations (with exceptions: templates,
inline functions).
it does not include what the implementation needs — only what the
declared interface requires.

the test: does this symbol appear in a function signature, a class
member declaration, or a base class? if yes, it belongs in the header.
if it appears only inside a function body, it belongs in the `.cpp`.

---

## transitive inclusion and why it is a problem

file A includes file B.
file B includes `<fstream>`.
file A can now use `std::ifstream` without including `<fstream>` itself.

this compiles. it is a bug in disguise.

why it is a bug:
- file A's dependency on `<fstream>` is invisible and implicit.
  someone reading file A cannot see where `std::ifstream` comes from.
- it is fragile. if file B is refactored to no longer need `<fstream>`,
  file A silently breaks. the dependency existed — it was just hidden.
- the fix is distant from the symptom. the error appears in file A;
  the cause is in file B's refactor.

rule: include at the site of use.
if file A uses `std::ifstream`, file A includes `<fstream>`.
its dependency is explicit, local, and stable under refactoring.

---

## the bundled-header anti-pattern

bundling all includes into a single `base.hpp` or `common.hpp` and
including that everywhere:

```cpp
// base.hpp
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iterator>
#include <map>
// ...
```

appears to simplify. in practice it creates 3 concrete problems.

### recompilation cascade

the compiler recompiles a translation unit when any included file
changes — directly or transitively.

if `base.hpp` is included by 10 `.cpp` files, any change to
`base.hpp` forces recompilation of all 10 translation units,
regardless of whether the change is relevant to 9 of them.

in projects with hundreds of translation units this becomes the
primary bottleneck in build time.

### dependency leakage

any file that includes `base.hpp` gains access to every symbol in
every header `base.hpp` pulls in. files use symbols without
declaring any dependency on them. the codebase accumulates invisible,
uncounted, unmaintainable dependencies.

### false interface contract

a header is a contract: "to use me, you need these types."
a bloated header makes a false contract: "to use me, you must
pay the cost of every implementation detail i ever needed."
every includer pays the full compilation cost of every symbol,
regardless of what it actually uses.

---

## correct discipline: 2 rules

### rule 1 — header includes only what its interface requires

the header includes the minimum set of headers such that the
declared signatures, types, and base classes are all complete
and computable by the compiler from the header alone.

`ConfigParser.hpp` includes `Config.hpp` (for `ServerConfig`,
`Location`, `ListenAddress` in method signatures), `<string>`,
and `<vector>`.
it does not include `<fstream>` — that is used only inside
`ConfigParser_0_read.cpp`'s implementation of `read()`.

### rule 2 — `.cpp` includes exactly what its implementation uses

each translation unit includes the headers for every symbol
it directly uses — no more, no less.

`ConfigParser_0_read.cpp` uses `std::ifstream`,
`std::istreambuf_iterator`, `std::runtime_error`, `std::string`.
it includes `<fstream>`, `<iterator>`, `<stdexcept>`, `<string>`.
it does not include `<map>` or `<optional>` — those are used in
other translation units.

---

## the webserv application

```
ConfigParser.hpp
    includes: Config.hpp, <string>, <vector>
    reason: these appear in declared signatures.

ConfigParser_0_read.cpp
    includes: ConfigParser.hpp, <fstream>, <iterator>,
              <stdexcept>, <string>
    reason: implementation uses file I/O, exception, string ops.
    does not include: <map>, <optional>, <sstream> — unused here.

ConfigParser_1_tokenise.cpp
    includes: ConfigParser.hpp, <string>
    reason: implementation uses std::string only.
    note: no <stdexcept> — tokenise does not throw.
    no <fstream> — no file I/O.

ConfigParser_2_parse.cpp
    includes: ConfigParser.hpp, <stdexcept>, <string>, <sstream>
    reason: parse methods throw, build strings, parse numbers.
```

each file's include list is a precise record of its dependencies.
reading it, you know exactly what the implementation touches
without reading the implementation.

---

## forward declarations — a further tool

when a header only needs to know a type exists (not its full
definition), a forward declaration suffices:

```cpp
class ServerConfig;   // forward declaration
```

vs.

```cpp
#include "Config.hpp" // full definition
```

a forward declaration is sufficient when:
- the type appears only as a pointer or reference in signatures.
- the type is not constructed, destructed, or have members accessed.

a full include is required when:
- the type is used by value (size must be known for layout).
- members or methods of the type are accessed.

forward declarations break include cycles and reduce compilation
dependencies further. they are not always applicable, but when
they are, prefer them.

---

## summary

```
#include = textual paste at compile time, not module import.

header = interface contract.
    includes: what the interface requires.
    does not include: what the implementation requires.

.cpp = implementation.
    includes: what it directly uses. nothing else.

bundled headers:
    cost: recompilation cascades, hidden dependencies,
          false contracts.
    benefit: apparent convenience. not real convenience.

rule: include at the site of use.
    explicit. local. stable under refactoring.
```