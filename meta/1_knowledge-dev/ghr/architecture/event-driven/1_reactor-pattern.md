# reactor pattern

## what problem necessitates this pattern

a server must respond to events arriving on multiple I/O sources simultaneously
— listen sockets becoming acceptable, client fds becoming readable or writable,
CGI pipes delivering output. these events are temporally unpredictable.

2 naive responses:

    blocking sequentially — read from each fd in turn. 
    problem: 1 fd that never becomes ready blocks all others. 
    structurally wrong.

    one thread per source — blocks per fd, but multiplied overhead, 
    shared state hazards, and scale ceiling. 
    rejected in the event-driven model.

the requirement is: wait efficiently on many sources, dispatch to the correct
handler when any source is ready, without blocking on any individual source.

this requirement has 1 structural solution.


---


## the pattern: 5 structural roles

the reactor pattern (Schmidt, 1995) defines 5 roles -
logical necessities given the requirement.

```
┌──────────────────────────────────────────────────────────────────┐
│                         dispatcher                               │
│                                                                  │
│   ┌─────────────────────────────────────────────────────────┐    │
│   │              synchronous event demultiplexer            │    │
│   │                   (epoll_wait)                          │    │
│   │                        │                                │    │
│   │   handle_set: { fd₁, fd₂, fd₃, ... }                    │    │
│   └─────────────────────────────────────────────────────────┘    │
│                            │                                     │
│                    on event for fdᵢ:                             │
│                            │                                     │
│   handler_registry[fdᵢ]────► concrete_handler.handle_event()     │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
                  ▲                            ▲
        ┌─────────┴────────┐       ┌───────────┴──────────┐
        │ concrete handler │       │  concrete handler    │
        │   (Listener)     │       │   (Connection)       │
        └──────────────────┘       └──────────────────────┘
```

### handle

an OS-managed token representing an I/O source. on Linux, a file descriptor.
the kernel tracks readiness state for each fd registered with epoll.

```cpp
// handles in webserv:
int listen_fd;    // accept events
int client_fd;    // read/write events
int cgi_pipe_fd;  // read events (CGI stdout)
```

handles are opaque integers from userspace's perspective. 
the demultiplexer is what makes them meaningful.

### synchronous event demultiplexer

a syscall that blocks until at least 1 handle in a registered set is ready,
then returns the ready subset. synchronous means: the call blocks the caller,
but the underlying wait is multiplexed — it watches all handles at once.

```c
int n = epoll_wait(epfd, events, MAX_EVENTS, timeout);
// blocks until ≥1 fd ready, returns n ready events
// each event identifies which fd and what condition
```

the kernel does the waiting. the application surrenders control until there
is work to do. this is the efficiency source: zero CPU consumed while waiting.

### handler interface (abstract event handler)

the type signature of a handler — the contract the dispatcher requires.
the dispatcher knows only this interface. it does not know Listener or
Connection at the dispatch site.

```cpp
class EpollHandler {
  public:
    virtual int      get_fd()          const = 0;
    virtual uint32_t get_events()      const = 0;
    virtual void     handle_event(uint32_t events) = 0;
    virtual bool     should_close()    const = 0;
    virtual std::string to_string()    const = 0;
};
```

the pure virtual functions are not a C++ convention. they are the logical
boundary: what must a handler be able to do, independent of what it is?
    — identify its fd (for registration and lookup)
    — declare which events it cares about
    — respond to an event
    — signal when it should be deregistered

### concrete event handlers

implementations of the handler interface, each containing the domain logic
for a specific event source type.

```cpp
// Listener — responds to EPOLLIN on a listen fd by calling accept()
class Listener : public EpollHandler { ... };

// Connection — responds to EPOLLIN/EPOLLOUT on a client fd
class Connection : public EpollHandler { ... };
```

the concrete handler is where domain meaning lives. the dispatcher has none
of this knowledge — it only knows EpollHandler*.

### dispatcher

the entity that owns the event loop: calls the demultiplexer, looks up the
handler for each ready fd, invokes handle_event. it is also responsible for
registering and deregistering handlers.

```cpp
// WebServ::run() — the dispatcher in webserv
void run() {
    struct epoll_event events[EPOLL_MAX_EVENTS];

    while (g_running) {
        int n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, EPOLL_TIMEOUT);
        // ... error handling ...

        for (int i = 0; i < n; ++i) {
            EpollHandler *handler =
                static_cast<EpollHandler *>(events[i].data.ptr);
            handler->handle_event(events[i].events);

            if (handler->should_close())
                remove_epoll_handler(handler->get_fd());
        }
    }
}
```

`events[i].data.ptr` is the mechanism by which the kernel delivers the
handler pointer alongside the event. the dispatcher stores `this` as the
epoll data pointer at registration time:

```cpp
ev.data.ptr = static_cast<void *>(new_epoll_handler.get());
```

so the round-trip is:
    registration: handler* → epoll_ctl → kernel associates ptr with fd
    dispatch:     kernel returns ptr with ready event → cast → handle_event()


---


## handler registry

the dispatcher must maintain a mapping from fd to handler. 
webserv uses a vector indexed by fd:

```cpp
std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;
// epoll_handlers[fd] → owner of that fd's handler
```

fds are non-negative integers. the vector index is the fd value.
this is O(1) lookup. the resize strategy:

```cpp
if (static_cast<std::size_t>(fd) >= epoll_handlers.size())
    epoll_handlers.resize(fd + EPOLL_HANDLERS_BATCH_SIZE);
```

grows in batches to amortise allocation. ownership is expressed via
unique_ptr — the registry owns each handler.

note: the ptr embedded in the epoll event is redundant with the registry
for lookup, but eliminates the lookup step — the kernel delivers the handler
directly. the registry is needed for ownership and deregistration.


---


## inversion of control

this is the defining structural characteristic of the reactor pattern.

in a sequential program, the application controls when I/O occurs:

```cpp
// proactive: program decides timing
read(fd, buf, n);    // application initiates
process(buf);
write(fd2, out, m);
```

in the reactor pattern, the application registers intent and cedes control:

```cpp
// reactive: kernel decides timing, application responds
register_handler(fd, handler);    // declare capability
event_loop();                     // surrender control to demultiplexer
// ... kernel calls back via handle_event() when fd ready
```

the application does not poll. it does not check. it waits, and is called.
this inversion is why it is called the reactor pattern: the application
reacts to events rather than proactively initiating them.

the consequence: all handler logic must be non-blocking. if handle_event()
blocks, the single thread is stalled and all other fds starve. non-blocking
I/O is not optional — it is structurally required by the model.


---


## registration lifecycle

```
1. create handler (Listener or Connection)
2. add_epoll_handler(std::move(handler))
       → epoll_ctl(EPOLL_CTL_ADD, fd, ev)     // register with kernel
       → epoll_handlers[fd] = handler          // register with registry
3. event loop runs; handler receives handle_event() calls
4. handler sets should_close() = true
5. dispatcher calls remove_epoll_handler(fd)
       → epoll_ctl(EPOLL_CTL_DEL, fd, nullptr) // deregister from kernel
       → epoll_handlers[fd].reset()            // destroy handler, close fd
```

the handler's destructor closes the fd via the Fd RAII wrapper. deregistration
from epoll before destruction is necessary — epoll will error on closed fds.


---


## relationship to proactor pattern

the reactor pattern is synchronous-reactive: the demultiplexer reports
readiness (fd is ready to read), and the handler performs the I/O itself.

the proactor pattern is asynchronous-proactive:
the application initiates an I/O operation, and is called back when the
operation completes (not merely when the fd is ready).

```
reactor:   kernel signals readiness  → application does I/O
proactor:  application initiates I/O → kernel signals completion (io_uring)
```

Linux io_uring implements the proactor model. webserv uses reactor via epoll.


---


## further reading

Schmidt, D.C. "Reactor: An Object Behavioral Pattern for Demultiplexing
    and Dispatching Handles for Synchronous Events." 1995.
    the original pattern paper. defines the five roles precisely.

Stevens, W.R. Unix Network Programming, Vol. 1. ch. 6.
    I/O models and multiplexing — the underlying syscall layer.

Gamma et al. Design Patterns. "Template Method" and "Strategy."
    handle_event() as template method; handler substitution as strategy.


---


## open questions

### dual-tracking: data.ptr and registry — is one redundant?

the dispatcher holds 2 references to each handler simultaneously:
    - data.ptr embedded in the epoll event (delivered by the kernel at dispatch)
    - epoll_handlers[fd] (the owning unique_ptr in the registry)

the ptr makes registry lookup unnecessary at dispatch time. but the registry
is still needed for ownership and deregistration. question: could a design
exist in which a single structure serves both roles — i.e., ownership managed
directly through the ptr mechanism, without a parallel vector?

this requires understanding: what is the minimal data structure for a reactor
registry, and what ownership model does it imply? research directions:
    - intrusive reference counting on handlers (shared_ptr stored in data.ptr)
    - registry as the sole owner, ptr as non-owning raw pointer (current design)
    - designs in libevent, libev, libuv — how do they solve this?