# Config Parser — Placement in Program

---

## Lukas's structure — principles

Two subdirectories under src/ and include/: base/ and classes/.

base/ is infrastructure: logging, formatting, signal handling,
global constants, stdlib includes. It has both headers and
implementation files. It has no knowledge of webserv's domain.
Nothing in base/ includes anything from classes/.

classes/ is domain objects: Server, Listener, and anything
added. Each class gets its own subdirectory under src/classes/,
with one .cpp file per method group or responsibility.
Everything in classes/ includes base/base.hpp.

Layering:

```
main.cpp
    |
classes/    (domain)
    |
base/       (infrastructure)
```

Dependency flows one way: classes depends on base.
base never depends on classes.

---

## Where the config parser lives

The parser is a domain component. It belongs in classes/.

1 file per pipeline phase, consistent with how Lukas
structured Server (1 file per method/responsibility).

---

## Where the config structs live: Config.hpp

The parser produces three types: ListenAddress, Location,
and the config data that populates Server objects.
These are pure data records — no methods, no behaviour.

Lukas's Server class is an operational object: it has
parse(), run(), start(), stop(). It contains config data
but is not merely config data.

These are different kinds of things:
- config structs: data, describing operator intent
- Server class: an actor with behaviour and lifecycle

A separate Config.hpp reflects this distinction:

```
include/classes/Config.hpp    — data records from config file
include/classes/Server.hpp    — operational object (Lukas's)
```

Config.hpp is included by:
- ConfigParser.hpp  (produces these types)
- Server.hpp        (consumes them)

Dependency direction: Server.hpp depends on Config.hpp.
Config.hpp depends on nothing in classes/.

### Team ownership boundary

Config.hpp is owned entirely by you.
Server.hpp is owned by Lukas.

Any change to config structs touches only Config.hpp.
No edit to Server.hpp is required during config parser
development. No merge conflicts on a file you share.

The file boundary maps to the team ownership boundary.
This is the criterion for where to draw the line.

---

## File creation checklist

New files to create:

    include/classes/Config.hpp
    include/classes/ConfigParser.hpp
    src/classes/ConfigParser/ConfigParser_read.cpp
    src/classes/ConfigParser/ConfigParser_tokenise.cpp
    src/classes/ConfigParser/ConfigParser_parse.cpp
    src/classes/ConfigParser/ConfigParser_validate.cpp

Edit required to Lukas's files:

    include/classes/Server.hpp
        — add: #include "classes/Config.hpp"
        — add Server config fields (ListenAddress, locations, etc.)
          once agreed with Lukas

Agree together before touching Server.hpp.