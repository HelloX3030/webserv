# event loop architecture


## current implementation (Lukas):

event loop currently lives in Server::run()


Header: include/classes/Server.hpp

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

Implementation: src/classes/Server/Server_run.cpp

```cpp
void Server::run() {
    start();
    while (g_running) {  // ← Infinite loop
        // commented-out accept/recv code
    }
    stop();
}
```

in action:

```cpp
WebServ::run() → Server[0].run() → poll(server0_fds)
              → Server[1].run() → poll(server1_fds)
```



## where should it live?

### reasoning
The event loop must be at the **highest level of control** in the program 
because it coordinates **all asynchronous I/O across all components**.

```
Process (the entire webserv program)
  └─ Event Loop (single, centralised coordinator)
      ├─ Polls ALL file descriptors
      ├─ Dispatches to handlers
      └─ Maintains global state

NOT:
Process
  ├─ Server 1 with its own loop
  ├─ Server 2 with its own loop
  └─ Server 3 with its own loop
(WRONG)
```

Why?
Because file descriptor readiness is a global property 
detected by a single poll() call. You cannot split it.


### where to organise in program code:

```c++

// Option 1: main() function

int main(int argc, char** argv) {
    WebServ::parse(argc, argv);
    WebServ::initialize_sockets();
    
    // Event loop HERE
    while (g_running) {
        poll(all_fds);
        dispatch_events();
    }
}

// Option 2: WebServ namespace

namespace WebServ {
    void run() {
        // Event loop HERE
        while (g_running) {
            poll(all_fds);
            dispatch_events();
        }
    }
}

int main() {
    WebServ::parse();
    WebServ::run();  // Never returns until shutdown
}
```


Option 2 is cleaner - separates concerns, keeps main() minimal.

    Server class does NOT contain event loop. 
    Server contains configuration and routing logic.