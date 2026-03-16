## location

this directory (`1_webserv_v0-v1`) belongs in
`./meta/1_knowledge-dev/architecture/src_organisation/`
alongside `0_general/`.

temporarily placed here for direct access during reorganisation,
to enable more easily a concurrent overview of the source code.

---

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

see `./meta/1_knowledge-dev/architecture/src_organisation/0_general/`
for upstream principles (WIP).

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

---

## architectural debt

`net/Connection` owns `HttpParser` from `http/`.
this couples net/ to the HTTP protocol.

in a purely layered design, net/ would be protocol-agnostic:
Connection handles bytes; a separate layer interprets them.

for webserv v1 (HTTP only), coupling is acceptable.
for (ghrod's) v2 (multi-protocol), refactor: inject protocol handler via interface.
