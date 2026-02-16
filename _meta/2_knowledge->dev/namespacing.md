## current implementation (Lukas)

WebServ namespace - top-level module
```cpp
// include/WebServ.hpp
namespace WebServ
{
    extern std::vector<Server> servers;  // Module state
    void parse(int argc, char **argv);   // Interface
    void run();                          // Interface
}

// src/classes/WebServ/WebServ.cpp
namespace WebServ
{
    std::vector<Server> servers;  // Definition
}
```

## issue

**external linkage exposes implementation.**

`extern` makes the variable declaration visible to all translation units:
```cpp
// Any file including WebServ.hpp can:
#include "WebServ.hpp"

WebServ::servers.clear();           // Direct mutation
WebServ::servers[0].some_method();  // Bypass interface
```

Consequences:
1. Storage implementation (vector) becomes part of public contract
2. Cannot change to `std::map<int, Server>` without breaking clients
3. No control over access - any code can modify state

Encapsulation violated: interface should expose *behaviour* (functions), 
not *representation* (data structures).

## fix

**use anonymous namespace for internal linkage.**
```cpp
// include/WebServ.hpp (unchanged public interface)
namespace WebServ
{
    void parse(int argc, char **argv);
    void run();
}

// src/classes/WebServ/WebServ.cpp
namespace WebServ
{
    namespace {  // Anonymous - internal to this translation unit
        std::vector<Server> servers;
        std::map<int, Connection> connections;
        std::vector<pollfd> pollfds;
    }
    
    void parse(int argc, char **argv) {
        servers.clear();  // Accessible within WebServ implementation
    }
    
    void run() {
        // Event loop can access servers, connections, pollfds
    }
}
```

**Why anonymous namespace over `static`?**

Both achieve internal linkage. Anonymous namespace preferred because:
```cpp
// static works for variables
static std::vector<Server> servers;  ✓

// static doesn't work for types
static struct ServerConfig { };      ✗ (invalid C++)
namespace { struct ServerConfig { }; }  ✓

// static is C idiom, anonymous namespace is C++ idiom
```

Result: `WebServ::servers` no longer accessible outside `WebServ.cpp`. 
Only `parse()` and `run()` are public interface.

## additional namespaces required

**HTTP parsing utilities** (pure functions, no state)
```cpp
namespace HTTP {
    Request parse_request(const std::string& raw);
    Response build_response(int status, const std::string& body);
}
```

**Config parsing** (currently methods on Server class - should extract)
```cpp
namespace Config {
    std::vector<Server> parse(const std::string& filepath);
}
```

**Event loop implementation details** (helper functions for WebServ::run)
```cpp
namespace WebServ {
    namespace {  // Internal helpers
        void handle_accept(int listen_fd);
        void handle_client_read(int client_fd);
        void handle_client_write(int client_fd);
    }
}
```