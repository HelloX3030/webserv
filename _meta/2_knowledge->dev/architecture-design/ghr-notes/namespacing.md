## current implementation (Lukas):

WebServ namespace - top-level module
encapsulating server state & event loop

Location: include/WebServ.hpp, src/classes/WebServ/*.cpp

```cpp
// WebServ.hpp
namespace WebServ
{
    extern std::vector<Server> servers;     // Module state
    void parse(int argc, char **argv);    // Interface
    void run();                           // Interface
}

// WebServ.cpp
namespace WebServ
{
    std::vector<Server> servers;  // Definition
}
```


## issue: external linkage breaks encapsulation 
(the whole point of using namespace pattern)

```cpp
extern std::vector<Server> servers;  // Accessible from anywhere
```

1. `extern` in header → declaration visible to all translation units
2. Any file including `WebServ.hpp` can write: `WebServ::servers.clear()`
3. Implementation details (how servers are stored) leak into public interface
4. Cannot change storage mechanism without breaking dependent code


Encapsulation means: *users call functions, not access data structures*.

    With external linkage: `WebServ::servers` IS the interface (data exposure)
    With internal linkage: `WebServ::parse()`, `WebServ::run()` ARE the interface (behaviour exposure)



### fix: use anonymous namespace

```cpp
// WebServ.cpp (implementation file)
namespace WebServ {
    namespace {  // Anonymous - internal linkage, file-local
        std::vector<Server> servers;
        std::map<int, Connection> connections;
        std::vector<pollfd> pollfds;
    }
    
    void parse(int argc, char **argv) {
        servers.clear();  // Can access anonymous namespace within WebServ
        // ...
    }
}
```

Why anonymous namespace over `static`:
- Consistent with modern C++ idiom
- Works for types, not just variables
- Clearer intent: "this is implementation detail"









# information to process on namespacing in rest of program



Which namespacing patterns should we use?

### Named Namespace with Internal State

Used for: WebServ module state (servers, connections, pollfds)
Why: We need persistent state across function calls

Implementation:

```cpp  
namespace WebServ {
      static std::vector<Server> servers;
      static std::map<int, Connection> connections;
      static std::vector<pollfd> pollfds;
      
      void parse(int argc, char **argv);
      void run();  // Contains THE event loop
  }
```

### Namespace as Module Interface

Used for: Utility namespaces (HTTP parsing, config parsing)
Why: Pure functions, no shared state

Implementation:

```cpp  
namespace HTTP {
      Request parse_request(const std::string& raw);
      Response build_response(int status, const std::string& body);
  }
```




No other namespaces yet - Missing:

    HTTP parsing utilities
    Config parsing (currently methods on Server class)
    Event loop implementation details