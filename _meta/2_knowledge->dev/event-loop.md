# Event loop architecture

## current implementation (Lukas)

Event loop lives in each Server instance.
```cpp
// src/classes/WebServ/WebServ_run.cpp
namespace WebServ
{
    void run()
    {
        for (std::size_t i = 0; i < servers.size(); i++)
        {
            servers[i].run();  // Call each Server's event loop
        }
    }
}

// include/classes/Server.hpp
class Server {
private:
    std::vector<Listener> listener;
    void start();
    void respond();
    void stop();
public:
    void run();  // Contains event loop
};

// src/classes/Server/Server_run.cpp
void Server::run() {
    start();
    while (g_running) {  // ← Infinite loop - blocks here
        // Event handling code (commented out currently)
    }
    stop();
}
```

Execution flow:
```
WebServ::run() 
  → servers[0].run() 
      → start()
      → while (g_running) { }  ← Blocks indefinitely
      → stop()                 ← Never reached (loop is infinite)
  → servers[1].run()           ← Never reached (stuck in servers[0])
  → servers[2].run()           ← Never reached
```

**Consequence:** Only `servers[0]` ever runs. All other servers unreachable.

## issue

**Cannot multiplex I/O with multiple blocking calls.**

### poll() system call semantics
```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

**Behaviour:** Blocks calling thread until ANY fd in array becomes ready.
```cpp
// What happens with current architecture:

// Server 0
void Server::run() {
    while (true) {
        poll(this->fds, ...);  // ← Blocks here waiting for this server's fds
                               // Cannot proceed until event occurs
                               // Then loops back and blocks again
    }
}

// Server 1 (never reached)
void Server::run() {
    while (true) {
        poll(this->fds, ...);  // ← Unreachable - stuck in Server 0's loop
    }
}
```

The spec requires serving **multiple servers simultaneously**. 
This architecture makes it impossible.

### Why multiple poll() calls don't work

Even if we fixed the infinite loop issue:
```cpp
// Attempt 1: Sequential polling
for (auto& server : servers) {
    poll(server.fds, timeout=0);  // Non-blocking
}
// Problem: Busy-waiting. CPU spins checking each server in sequence.
// Wastes cycles, high CPU usage, poor performance.

// Attempt 2: Separate threads (not allowed by subject)
for (auto& server : servers) {
    std::thread([&]() { server.run(); }).detach();
}
// Problem: Violates subject constraints. No threading allowed.
```

### Logical necessity

**poll() multiplexes N file descriptors by accepting ALL N in a single call.**
```cpp
// Correct: One poll() for all fds
std::vector<pollfd> all_fds;
all_fds.push_back({server0_listen_fd, POLLIN, 0});
all_fds.push_back({server1_listen_fd, POLLIN, 0});
all_fds.push_back({server2_listen_fd, POLLIN, 0});
all_fds.push_back({client_fd_1, POLLIN, 0});
all_fds.push_back({client_fd_2, POLLIN, 0});

poll(all_fds.data(), all_fds.size(), -1);  // Wait for ANY fd
// When ANY fd becomes ready, poll() returns
// Process ready fds, then poll() again
```

**Constraint:** To monitor N fds concurrently, pass all N to ONE poll() call.

**Consequence:** Event loop must be singular and own all fds from all servers.

## fix

**Event loop at WebServ namespace level, polling all fds from all servers.**
```cpp
// include/WebServ.hpp
namespace WebServ
{
    void parse(int argc, char **argv);
    void run();  // THE event loop (never returns until shutdown)
}

// src/classes/WebServ/WebServ_run.cpp
namespace WebServ
{
    namespace {
        std::vector<Server> servers;
        std::map<int, Connection> connections;
        std::vector<pollfd> pollfds;
        std::map<int, Server*> listen_fd_to_server;  // Map listen fd → owning Server
        
        bool is_listen_fd(int fd) {
            return listen_fd_to_server.find(fd) != listen_fd_to_server.end();
        }
        
        void handle_listen_fd(int fd) {
            Server* server = listen_fd_to_server[fd];
            int client_fd = accept(fd, NULL, NULL);
            
            if (client_fd < 0) return;
            
            // Add client fd to poll set
            pollfds.push_back({client_fd, POLLIN, 0});
            
            // Create Connection object
            connections[client_fd] = Connection(client_fd, server);
        }
        
        void handle_client_fd(int fd) {
            Connection& conn = connections[fd];
            
            if (pollfds[i].revents & POLLIN) {
                conn.read_request();
            }
            if (pollfds[i].revents & POLLOUT) {
                conn.write_response();
            }
            
            if (conn.is_complete()) {
                close(fd);
                connections.erase(fd);
                // Remove from pollfds
            }
        }
    }
    
    void run()
    {
        // Build pollfd array from ALL servers' listening sockets
        for (auto& server : servers) {
            server.create_listening_sockets();  // Server creates its listen fds
            
            for (int listen_fd : server.get_listen_fds()) {
                pollfds.push_back({listen_fd, POLLIN, 0});
                listen_fd_to_server[listen_fd] = &server;
            }
        }
        
        // Single event loop for entire process
        while (g_running) {
            int ready = poll(pollfds.data(), pollfds.size(), -1);
            
            if (ready < 0) {
                if (errno == EINTR) continue;  // Interrupted by signal
                throw std::runtime_error("poll() failed");
            }
            
            // Process ready fds
            for (size_t i = 0; i < pollfds.size(); i++) {
                if (pollfds[i].revents == 0) continue;  // No events
                
                int fd = pollfds[i].fd;
                
                if (is_listen_fd(fd)) {
                    handle_listen_fd(fd);  // New connection
                } else {
                    handle_client_fd(fd);  // Existing connection
                }
            }
        }
    }
}
```

### Server class role change

Server no longer contains event loop. Server is configuration + routing logic.
```cpp
// include/classes/Server.hpp
class Server {
private:
    std::string host;
    uint16_t port;
    std::vector<int> listen_fds;
    std::map<std::string, Location> routes;
    
public:
    // Configuration
    void parse(const std::string& config_path);
    
    // Socket setup (called once during initialization)
    void create_listening_sockets();
    std::vector<int> get_listen_fds() const { return listen_fds; }
    
    // Request routing (called per request)
    Response route_request(const Request& req);
    
    // NO run() method - no event loop here
};
```

### Separation of concerns

| Component | Responsibility |
|-----------|---------------|
| `main()` | Program lifecycle: init → run → cleanup |
| `WebServ::run()` | I/O multiplexing: poll all fds, dispatch events |
| `Server` | Configuration + routing: match URI → response |
| `Connection` | Request/response state: per-client lifecycle |

### Execution flow
```
main()
  ↓
WebServ::parse()
  → Parse config files
  → Build Server objects
  ↓
WebServ::run()
  → Call server.create_listening_sockets() for each server
  → Build pollfd array from all listen fds
  → Enter infinite loop:
      while (g_running) {
          poll(all_fds)              ← ONE poll() for ALL servers
          for each ready fd:
              if listen_fd:
                  accept() → create Connection
              if client_fd:
                  route to appropriate Server
                  handle read/write
      }
```

**Key principle:** The event loop IS the server runtime. 
One loop, one poll() call, all fds.

### What Server knows
```cpp
Server knows:
- Its configuration (host, port, routes)
- How to create listening sockets
- How to route requests to responses

Server does NOT know:
- When connections arrive (event loop's job)
- Client connection state (Connection's job)
- Other servers' fds (WebServ's job)
```

Server is data structure + pure functions. 
No control flow, no loops, no I/O multiplexing.