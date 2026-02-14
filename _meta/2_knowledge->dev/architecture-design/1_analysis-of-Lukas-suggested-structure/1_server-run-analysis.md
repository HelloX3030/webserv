## current design:

extracted from...:

...Header:
include/classes/Server.hpp

```cpp
class Server {
  private:
    std::vector<Listener> listener;
    void start();
    void respond();
    void stop();
  public:
    void run();
};
```

...Implementation: 
src/classes/Server/Server_run.cpp

```cpp
void Server::run() {
    start();
    while (g_running) {  // ← Infinite loop
        // commented-out accept/recv code
    }
    stop();
}
```


What does this mean?

1. Each Server runs its own event loop?

    Implies threading or sequential execution
    Can't poll multiple servers simultaneously (each run() blocks)
    Wastes resources (N servers = N event loops = N poll() calls per iteration)
    Violates spec ("single poll()")

2. scaffolding - Server::run() is temporary structure for early testing??

    Just echoes back bytes
    Will be replaced by centralized loop later




## my reasoning

Start with "one poll() call must monitor all fds" 
→ derive structure from this constraint 
→ event loop at top, Server is just data + routing logic.



Server's job:

    Parse config
    Create listening sockets
    Route requests (match URI to Location)
    Generate responses


Server does NOT:

    Run event loop
    Track client connections
    Poll file descriptors