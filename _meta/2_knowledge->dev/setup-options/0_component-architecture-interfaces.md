# Component architecture and interfaces

## current state

**Exists:**
- `WebServ` namespace (with external linkage issue - see namespacing.md)
- `Server` class (with event loop issue - see event-loop.md)
- `Listener` class - what's its purpose?

**Missing:**
- `Connection` class (client connection state)
- Clear interface contract between parser and runtime

**Unclear:**
- What data does Server contain?
- Where do Connection objects live?
- Why does Listener exist?



## issues


### 2: Connection class missing

Runtime needs per-client state:
- Read buffer (accumulate HTTP request)
- Write buffer (HTTP response to send)
- State machine (READING → PARSING → PROCESSING → WRITING)
- Associated Server (for routing)

Currently this doesn't exist.


### 3: Unclear ownership boundaries

**Questions:**
- Do Connection objects live in WebServ namespace (global pool) 
or Server class (per-server)?
- Does Server own its listen fds or does WebServ track them?
- Who creates Connection objects - WebServ on accept() or Server?

**This affects parser:** What data structures should parser build?



## architecture proposal

### Server class: Configuration + routing
```cpp
// include/classes/Server.hpp
class Server {
private:
    // Configuration (from parser)
    std::string host;
    uint16_t port;
    std::vector<std::string> server_names;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;        // 404 → "/errors/404.html"
    std::map<std::string, Location> locations;     // "/api/" → Location config
    
    // Runtime state
    std::vector<int> listen_fds;  // Created during initialization
    
public:
    // Configuration (parser calls this)
    void set_config(/* parsed config data */);
    
    // Socket management (WebServ::run() calls during init)
    void create_listening_sockets();
    std::vector<int> get_listen_fds() const;
    
    // Request routing (called per request)
    Response route_request(const Request& req) const;
    Location* match_location(const std::string& uri);
};

struct Location {
    std::string path_prefix;           // "/api/"
    std::string root;                  // "/var/www"
    std::vector<std::string> index_files;
    std::set<std::string> allowed_methods;
    bool autoindex;
    std::string cgi_extension;
    std::string cgi_path;
};
```

**Server responsibilities:**
- Store configuration
- Create listening sockets (during init)
- Route requests to responses (pure function: Request → Response)

**Server does NOT:**
- Track client connections
- Manage I/O buffers
- Run event loop


### Connection class: Per-client state
```cpp
// include/classes/Connection.hpp
class Connection {
private:
    int fd;
    Server* server;  // Which server this connection belongs to
    
    // State machine
    enum State { READING, PARSING, PROCESSING, WRITING, COMPLETE };
    State state;
    
    // Buffers
    std::string read_buffer;
    std::string write_buffer;
    size_t write_offset;  // How much of write_buffer sent
    
    // Parsed request
    Request request;
    
public:
    Connection(int fd, Server* server);
    
    // I/O operations (called by event loop)
    void read_from_socket();   // recv() → read_buffer
    void write_to_socket();    // write_buffer → send()
    
    // State transitions
    void parse_request();      // read_buffer → Request
    void process_request();    // Request → Response → write_buffer
    
    // State queries
    bool is_complete() const;
    bool wants_read() const;   // Should fd be polled for POLLIN?
    bool wants_write() const;  // Should fd be polled for POLLOUT?
};
```

**Connection responsibilities:**
- Manage per-client I/O buffers
- Track request/response state machine
- Parse HTTP request
- Generate HTTP response (via Server routing)


### WebServ namespace: Coordination
```cpp
// src/classes/WebServ/WebServ.cpp
namespace WebServ {
    namespace {
        std::vector<Server> servers;                    // Parsed from config
        std::map<int, Connection> connections;          // Active clients
        std::vector<pollfd> pollfds;                    // For poll()
        std::map<int, Server*> listen_fd_to_server;     // Map listen fd → Server
    }
    
    void parse(int argc, char **argv);  // Builds servers vector
    void run();                         // Event loop
}
```

**WebServ responsibilities:**
- Own all Server objects (from parser)
- Own all Connection objects (from accept())
- Run event loop
- Dispatch events to appropriate handlers


### Ownership and lifetime
```
WebServ namespace owns:
  ├─ servers (vector of Server objects)
  │    └─ Each Server contains:
  │         ├─ Configuration (from parser)
  │         └─ listen_fds (created during init)
  │
  └─ connections (map: fd → Connection)
       └─ Each Connection contains:
            ├─ Reference to Server (for routing)
            ├─ I/O buffers
            └─ State machine
```

**Lifetime rules:**
- Server objects created by parser, live for program lifetime
- Connection objects created on accept(), destroyed on close()
- Server* pointers in Connection remain valid (Server never destroyed)



## interface contracts

### Parser → Server

**Parser produces:**
```cpp
std::vector<Server> parse_config(const std::string& filepath);
```

**Server must contain after parsing:**
- All configuration from config file
- Empty listen_fds (filled later by create_listening_sockets())

**Example:**
```cpp
Server server;
server.host = "0.0.0.0";
server.port = 8080;
server.server_names = {"example.com", "www.example.com"};
server.client_max_body_size = 1048576;  // 1MB
server.error_pages[404] = "/errors/404.html";
server.locations["/"] = Location{...};
server.locations["/api/"] = Location{...};
// listen_fds is empty at this point
```


### Server → WebServ runtime

**After WebServ::parse():**
```cpp
// servers vector is populated with parsed Server objects
```

**During WebServ::run() initialization:**
```cpp
for (auto& server : servers) {
    server.create_listening_sockets();  // Server creates its own fds
    
    for (int fd : server.get_listen_fds()) {
        pollfds.push_back({fd, POLLIN, 0});
        listen_fd_to_server[fd] = &server;
    }
}
```

**During request handling:**
```cpp
void handle_client_fd(int fd) {
    Connection& conn = connections[fd];
    
    if (conn.wants_read()) {
        conn.read_from_socket();
        if (/* read complete */) {
            conn.parse_request();
            conn.process_request();  // Internally calls conn.server->route_request()
        }
    }
    
    if (conn.wants_write()) {
        conn.write_to_socket();
    }
}
```



## decisions required



### Connection objects in WebServ namespace?

**Proposal:** Yes. WebServ owns global connection pool.

**Rationale:**
- Event loop needs to iterate all connections
- Connection lifetime managed by accept()/close() in event loop
- Server is stateless configuration, doesn't track runtime state

**Implementation:**
```cpp
namespace WebServ {
    namespace {
        std::map<int, Connection> connections;  // fd → Connection
    }
}
```

### Parser output format?

**Proposal:** Parser returns `std::vector<Server>` with populated configuration fields.

**Rationale:**
- Simple interface: parse() → vector of data structures
- Runtime calls `server.create_listening_sockets()` to add listen_fds
- Clear separation: parser builds data, runtime initializes sockets

**Implementation:**
```cpp
// Parser
namespace Config {
    std::vector<Server> parse(const std::string& filepath);
}

// Usage in WebServ::parse()
servers = Config::parse(filepath);
```



## summary

Before I can implement configuration parser, we need to establish:

1. **Server class contents** (see Server class proposal above)
2. **Connection class design** (see Connection class proposal above)

4. **Parser output** (vector of Server objects with config populated)
5. **Ownership** (WebServ owns servers and connections)