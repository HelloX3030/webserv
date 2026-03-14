for event loop, connection lifecycle, runtime orchestration

## from v0 -> v1:

has integrated the previous:

WebServ/
namespace orchestrator with global state (epfd, servers, epoll_handlers)

Server/
thin wrapper around ServerConfig + add_server factory

base/signal
g_running and handle_sigint encode WebServ-specific shutdown semantics.
belongs with runtime orchestration, not base/.
