# config parser — placement

---

## location in codebase

domain component → belongs in classes/.

```
include/classes/
    Config.hpp          — data records (ghr ownership)
    ConfigParser.hpp    — parser declaration (ghr ownership)
src/classes/ConfigParser/
    ConfigParser_1_read.cpp
    ConfigParser_2_tokenise.cpp
    ConfigParser_3_parse.cpp
    ConfigParser_4_validate.cpp
```

file numbering reflects pipeline order explicitly.

---

## Config.hpp ownership boundary

Config.hpp belongs to ghr. Server.hpp to Lukas.
changes to config structs touch only Config.hpp.
no edits to Server.hpp required during config parser development.
file boundary = team ownership boundary. no merge conflicts.

---

## edit required to Lukas's files

include/classes/Server.hpp:
    add: #include "classes/Config.hpp"
    add: ServerConfig or its fields as member(s)

see interface-for-lukas.md for the handoff contract.