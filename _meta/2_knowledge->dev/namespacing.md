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
            servers[i].run();  // Each Server has its own event loop
        }
    }
}

// Implied Server class structure:
class Server {
public:
    void run() {
        // Event loop for this server's fds
        while (running) {
            poll(this->fds);
            handle_events();
        }
    }
};
```

Execution flow:
```
WebServ::run() → Server[0].run() → poll(server0_fds) → never returns
              → Server[1].run() → unreachable
```

## issue

**Cannot multiplex I/O with multiple blocking calls.**

`poll()` system call signature:
```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

Behaviour: Blocks calling thread until ANY fd in array becomes ready.

With multiple poll() calls:
```cpp
// Server 0
poll(fds_server0);  // Blocks here waiting for server0's fds
                    // Cannot proceed to server1 until event occurs

// Server 1 (never reached)
poll(fds_server1);  // Unreachable - stuck in server0's loop
```

The spec requires serving **multiple servers simultaneously**. 
This architecture makes it impossible:
- Server 0 gets infinite loop
- Servers 1, 2, 3... never initialize

Even if we fixed the loop issue, multiple `poll()` calls would serialize I/O:
```cpp
for (auto& server : servers) {
    poll(server.fds, timeout=0);  // Non-blocking
}
// Problem: Now we busy-wait, burning CPU checking each server in sequence
```

**Logical necessity:** Multiplexing N file descriptors 
requires passing ALL N to a SINGLE `poll()` call.

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
        
        void handle_listen_fd(int fd);
        void handle_client_fd(int fd);
    }
    
    void run()
    {
        // Build pollfd array from ALL servers
        for (auto& server : servers) {
            for (int listen_fd : server.get_listen_fds()) {
                pollfds.push_back({listen_fd, POLLIN, 0});
            }
        }
        
        // Single event loop for entire process
        while (g_running) {
            int ready = poll(pollfds.data(), pollfds.size(), -1);
            
            if (ready < 0) {
                if (errno == EINTR) continue;
                throw std::runtime_error("poll() failed");
            }
            
            for (auto& pfd : pollfds) {
                if (pfd.revents & POLLIN) {
                    if (is_listen_fd(pfd.fd)) {
                        handle_listen_fd(pfd.fd);
                    } else {
                        handle_client_fd(pfd.fd);
                    }
                }
            }
        }
    }
}
```

**Server class role changes:**
```cpp
class Server {
private:
    std::vector<int> listen_fds;  // Created during init
    std::map<std::string, Location> routes;
    
public:
    void create_listening_sockets();  // Called once during setup
    std::vector<int> get_listen_fds() const;
    
    Response route_request(const Request& req);  // Called per request
    
    // NO run() method - no event loop here
};
```

Separation of concerns:

| Component | Responsibility |
|-----------|---------------|
| `main()` | Program lifecycle (init → run → cleanup) |
| `WebServ::run()` | I/O multiplexing (poll all fds, dispatch events) |
| `Server` | Configuration + routing (match URI → response) |
| `Connection` | Request/response state machine (per-client state) |

Execution flow:
```
main()
  ↓
WebServ::parse()     → Build Server objects from config
  ↓
WebServ::run()       → Create listen sockets from all servers
  ↓                  → Enter infinite loop:
while (g_running) {      poll(all_fds_from_all_servers)
                         dispatch_to_handlers()
                     }
```

The event loop IS the server runtime. 
There is no higher-level loop.