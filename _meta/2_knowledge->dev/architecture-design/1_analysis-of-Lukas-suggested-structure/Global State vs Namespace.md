## Global state:

```cpp
// Global scope (bad practice in C++)
std::vector<Server> g_servers;
std::map<int, Connection*> g_connections;
std::vector<pollfd> g_pollfds;

int main() {
    // Can access g_servers directly
}
```

Problems:

    Pollutes global namespace
    Name collisions possible
    No access control
    Hard to reason about ownership



## Namespace:

```cpp
namespace WebServ {
    // File-scope static (internal linkage)
    static std::vector<Server> servers;
    static std::map<int, Connection*> connections;
    static std::vector<pollfd> pollfds;
    
    void parse() { /* access servers */ }
    void run() { /* access connections */ }
}

int main() {
    WebServ::parse();
    WebServ::run();
    // Cannot directly access WebServ::servers (encapsulated)
}
```

Advantages:

    Encapsulation (implementation detail hidden)
    Explicit interface (only functions in namespace are public)
    No global namespace pollution
    Clear ownership (WebServ namespace owns this state)



Best practice: Use namespace with internal-linkage variables 
(static or anonymous namespace).