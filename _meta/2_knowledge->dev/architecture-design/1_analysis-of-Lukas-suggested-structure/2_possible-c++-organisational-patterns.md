## Pattern 1: Namespace with Free Functions (Simplest)

```cpp
namespace WebServ {
    static std::vector<Server> servers;
    static std::map<int, Connection> connections;
    
    void parse_config(const char* path);
    void initialize_listening_sockets();
    void event_loop();
    void handle_accept(int listen_fd);
    void handle_client(Connection& conn);
}
```

Advantages:

    Direct, clear data flow
    No unnecessary abstraction
    Easy to reason about
    C-style but type-safe


## Pattern 2: EventLoop Class (OOP)

```cpp
class EventLoop {
private:
    std::vector<pollfd> fds;
    std::map<int, Connection> connections;
    std::map<int, Server*> listen_fd_map;
    
public:
    void register_listener(int fd, Server* server);
    void run();
    
private:
    void handle_listen_fd(int fd);
    void handle_client_fd(int fd);
};

int main() {
    std::vector<Server> servers = parse_config();
    
    EventLoop loop;
    for (auto& server : servers) {
        for (int fd : server.create_listening_sockets()) {
            loop.register_listener(fd, &server);
        }
    }
    
    loop.run();
}
```

Advantages:

    Clear ownership (EventLoop owns the loop state)
    Testable (can mock EventLoop)
    Extensible (can subclass, add hooks)


## Pattern 3: Reactor Pattern (Advanced)

```cpp
class Reactor {
public:
    void register_handler(int fd, EventHandler* handler);
    void run();
};

class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual void handle_read() = 0;
    virtual void handle_write() = 0;
};

class ListenHandler : public EventHandler {
    Server* server;
    Reactor* reactor;
public:
    void handle_read() override {
        int client_fd = accept(...);
        reactor->register_handler(client_fd, new ConnectionHandler(...));
    }
};

class ConnectionHandler : public EventHandler {
    Connection conn;
public:
    void handle_read() override { /* read request */ }
    void handle_write() override { /* send response */ }
};
```

Advantages:

    Industry-standard pattern
    Very extensible
    Clear separation of concerns
    Used in ACE, Boost.Asio, libevent

Disadvantages:

    More complex
    More abstraction overhead
    Overkill for webserv project


Recommendation

For webserv project:
Use Pattern 1 (namespace + free functions) or Pattern 2 (EventLoop class).

Why:

    Simplicity: Spec demands correctness, not architectural sophistication