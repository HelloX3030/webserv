# config parser — placement

---

## location in codebase

domain component → belongs in classes/.

```
include/classes/
    Config.hpp          — data records (ghr ownership)
    ConfigParser.hpp    — parser declaration (ghr ownership)
src/classes/ConfigParser/
    .cpp files          — file numbering reflects pipeline order explicitly.
```

---

## Config.hpp ownership boundary

Config.hpp belongs to ghr. Server.hpp to Lukas.
changes to config structs touch only Config.hpp.
no edits to Server.hpp required during config parser development.
file boundary = team ownership boundary. no merge conflicts.