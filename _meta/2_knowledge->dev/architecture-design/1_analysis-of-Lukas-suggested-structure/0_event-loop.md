# Event loop

## Where should it live?

The event loop must be at the **highest level of control** in the program 
because it coordinates **all asynchronous I/O across all components**.

```
Process (the entire webserv program)
  └─ Event Loop (single, centralized coordinator)
      ├─ Polls ALL file descriptors
      ├─ Dispatches to handlers
      └─ Maintains global state

NOT:
Process
  ├─ Server 1 with its own loop ← WRONG
  ├─ Server 2 with its own loop ← WRONG
  └─ Server 3 with its own loop ← WRONG
```

Why?
Because file descriptor readiness is a global property 
detected by a single poll() call. You cannot split it.

## where exactly in prog?
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