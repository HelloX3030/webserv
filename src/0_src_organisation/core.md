## predicate

lifecycle orchestrator.
init → run → quit.
global state ownership. event loop dispatch.

---

## contents

```
WebServ.cpp             namespace definition
WebServ_init.cpp        startup, listener registration
WebServ_run.cpp         event loop
WebServ_quit.cpp        shutdown, cleanup
WebServ_load_config.cpp config file → ServerConfig
WebServ_display.cpp     debug output
Server.cpp              thin wrapper, add_server factory
signal.cpp              g_running, handle_sigint
```

---

## naming

chosen: "core"
- central orchestrator, everything flows through
- honest about structural role: centre of dependency graph
- generic, but accurately so — it *is* the core

rejected:

| name     | problem                                            |
|----------|----------------------------------------------------|
| app/     | vague                                              |
| server/  | overloaded — the whole program is a server         |
| runtime/ | suggests execution environment, sounds like "C++ runtime" |
| loop/    | too narrow — init/quit aren't "loop"               |

---

## v0 → v1

integrates:

`classes/WebServ/`
- namespace orchestrator with global state (epfd, servers, epoll_handlers)

`classes/Server/`
- thin wrapper around ServerConfig + add_server factory
- nearly a typedef
- WebServ::add_server does the work; physical placement now matches namespace

`base/signal.cpp`
- g_running and handle_sigint are shutdown semantics
- tied to lifecycle, not domain-agnostic
- base/ test: "could another project use this unchanged?" — no
