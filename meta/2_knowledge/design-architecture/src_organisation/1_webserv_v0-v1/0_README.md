## v0

initial structure to begin development.

categories:
- `base/` — foundational infrastructure, utilities
- `classes/`
- expanded to include `interfaces/`

syntactic grouping. no semantic constraint.
once `classes/` bloated, reorganisation became necessary.

---

## v1

principle: semantic-logical cohesion.
group by domain, concern, or structural role.

each category has a markdown file documenting:
reasoning on name, contents, inclusion/exclusion from v0.

---

## categories

```
base/       dependency floor. domain-agnostic primitives.
core/       lifecycle orchestrator. global state. event loop.
net/        reactor infrastructure. sockets, epoll, connections.
http/       protocol layer. request parsing, response building, routing.
handlers/   application behaviour. method implementations.
config/     configuration frontend. read, parse, validate.
cgi/        process execution. fork, exec, pipes, environment.
main.cpp    entry point. top-level control flow.
```

---

## naming axes

categories are named by different criteria:

| category   | axis                |
|------------|---------------------|
| base/      | position (floor)    |
| core/      | role (orchestrator) |
| net/       | domain (networking) |
| http/      | protocol            |
| handlers/  | role (processors)   |
| config/    | concern             |
| cgi/       | technology          |

each name reflects what most precisely identifies the category's boundary.

---

## conventions

naming reflects semantic distinction: instantiable vs container.

| entity              | nature              | convention |
|---------------------|---------------------|------------|
| class, struct, enum | type (instantiable) | PascalCase |
| namespace           | scope (container)   | lowercase  |
| directory           | scope (container)   | lowercase  |

files named after what they declare:
- `Fd.cpp` → declares `Fd` class
- `format.cpp` → declares `format` namespace

subdirectories: don't repeat parent domain (filesystem provides ctx).

examples:
- v0: `ConfigFrontend/ConfigFrontend_1_tokenise.inc`
- v1: `config/frontend/tokenise.inc`


### additional

inc/          public interface surface. mirrors src/ top-level domains only.
              internal src/ subdivisions (request-frontend/, frontend/) are
              NOT mirrored. include paths encode domain, not implementation.

class-split files:
when a single entity's definition exceeds 1 file, split files follow:
  EntityName.cpp          primary definitions or compilation target
  EntityName_concern.cpp  definitions for specific concern group
EntityName matches the declaration file. concern identifies the function group.
applied: WebServ_init, WebServ_run, HttpMethods_delete, etc.


---

## architectural debt

`net/Connection` owns `HttpParser` from `http/`.
this couples net/ to the HTTP protocol.

in a purely layered design, net/ would be protocol-agnostic:
Connection handles bytes; a separate layer interprets them.

for webserv v1 (HTTP only), coupling is acceptable.
for (ghrod's) v2 (multi-protocol), refactor: inject protocol handler via interface.
