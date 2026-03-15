# core

## what this directory owns

Lifecycle phases: init → run → quit
Global state: epfd, servers, epoll_handlers
Event loop dispatch (runtime orchestration)

## from v0 -> v1:

Previous categories which this 1 new category integrates:


- classes/WebServ/
  namespace orchestrator with global state (epfd, servers, epoll_handlers)


- classes/Server/
  thin wrapper around ServerConfig + add_server factory
  nearly a typedef.

  The WebServ::add_server function does the actual work (listener registration) and lives in Server.cpp
  but belongs to WebServ namespace. namespace coupling already declares ownership. Physical placement should match logical ownership.

  Dissolved into same directory as WebServ/


- g_running and handle_sigint are shutdown semantics.
  They exist because of the lifecycle (init → run → quit).
  The event loop checks g_running. The signal handler sets it.
  This is orchestration state, not domain-agnostic infrastructure.

  The base/ test: "could another project use this unchanged?"
  No — another server might handle shutdown differently,
  use different signals, have different graceful-shutdown logic.


## naming

### considered also:

app/
application layer
- vague

server/
- overloaded — the whole program is a server

runtime/
+ lifecycle-oriented, contrasts with parse-time
- suggests an execution environment, not orchestration
  sounds like "C++ runtime"

loop/
+ direct — it's the event loop
- too narrow (init/quit aren't "loop")

### chosen: core/

+ means, implies: the central orchestrator, everything flows through
- generic: says nothing about what kind of core

is honest about the structural role: this is the center of the dependency graph,
the sequencer, the owner of global state. It doesn't claim domain semantics it doesn't have.

The lifecycle pattern (init → run → quit) is a core pattern.
The event loop is the core loop. The global state is the core state.
