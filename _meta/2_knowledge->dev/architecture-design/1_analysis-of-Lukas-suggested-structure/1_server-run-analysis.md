Current design:

```cpp
class Server {
    std::vector<Listener> listener;
    void run();  // Infinite loop
};


Server (class)
  ├─ vector<Listener>    // Its listening sockets
  ├─ run()               // Its event loop
```



What does this mean?

1. "Each Server runs its own event loop"

    Implies threading or sequential execution
    Can't poll multiple servers simultaneously (each run() blocks)
    Wastes resources (N servers = N event loops = N poll() calls per iteration)
    Violates spec ("single poll()")

2. scaffolding: "Server::run() is temporary structure for early testing"??

    Just echoes back bytes
    Will be replaced by centralized loop later
    Reasonable for incremental development




Server's job:

    Parse config
    Create listening sockets
    Route requests (match URI to Location)
    Generate responses


Server does NOT:

    Run event loop
    Track client connections
    Poll file descriptors