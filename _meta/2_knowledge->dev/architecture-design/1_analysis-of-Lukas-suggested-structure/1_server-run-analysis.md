
```c++
class Server {
    std::vector<Listener> listener;
    void run();  // Infinite loop
};
```

What does this mean?

1. "Each Server runs its own event loop"

    Implies threading or sequential execution
    Violates spec ("single poll()")
    incorrect

2. scaffolding: "Server::run() is temporary structure for early testing"

    Just echoes back bytes
    Will be replaced by centralized loop later
    Reasonable for incremental development


Many tutorials show simple servers like:
```cpp
Server server;
while (1) {
    client = server.accept();
    server.handle(client);
}
```

This works for toy examples but fundamentally doesn't scale to concurrent connections.

Event loop must be above Server abstraction.