# polymorphic dispatch

## the problem

the dispatcher in `WebServ::run()` holds a pointer to a handler and calls
`handle_event()`. but what code should execute when that call is made?

```cpp
EpollHandler *handler = static_cast<EpollHandler *>(events[i].data.ptr);
handler->handle_event(events[i].events);
```

the dispatcher does not know whether this pointer points to a Listener or a
Connection. nor should it — the loop's job is to dispatch, not to encode
knowledge of every handler type. if the loop contained:

```cpp
if (is_listener(handler))
    static_cast<Listener *>(handler)->handle_event(events);
else if (is_connection(handler))
    static_cast<Connection *>(handler)->handle_event(events);
```

then adding any new handler type requires modifying the dispatcher. the
loop and the handler types are coupled. this is structurally wrong.

the requirement: given a pointer to a base type, invoke the correct
derived-type behaviour, without the caller knowing the derived type.

this is the problem polymorphic dispatch solves.


---


## the mechanism: virtual dispatch

C++ resolves this via the virtual function mechanism. when a method is
declared `virtual` in a base class and overridden in a derived class, the
call through a base pointer invokes the derived class's version.

```cpp
// base
class EpollHandler {
    virtual void handle_event(uint32_t events) = 0;
};

// derived
class Listener : public EpollHandler {
    void handle_event(uint32_t events) override;  // accept() logic
};

class Connection : public EpollHandler {
    void handle_event(uint32_t events) override;  // read/write logic
};

// dispatch site
EpollHandler *h = /* points to a Listener or Connection */;
h->handle_event(mask);   // correct derived implementation called
```

the caller — `WebServ::run()` — is written once, against the base type.
the correct implementation is selected at runtime based on the actual type
of the object.


---


## the vtable: how virtual dispatch works

virtual dispatch is implemented via a hidden per-class table of function
pointers: the vtable (virtual function table).

for each class with virtual methods, the compiler generates one vtable —
a static array of function pointers, one per virtual method, in declaration
order. each instance of the class contains a hidden pointer (the vptr) to
its class's vtable, inserted by the compiler at the start of the object.

```
EpollHandler vtable:
    [0] → EpollHandler::get_fd        (= 0, pure virtual — no entry)
    [1] → EpollHandler::get_events    (= 0)
    [2] → EpollHandler::handle_event  (= 0)
    [3] → EpollHandler::should_close  (= 0)
    [4] → EpollHandler::to_string     (= 0)

Listener vtable:
    [0] → Listener::get_fd
    [1] → Listener::get_events
    [2] → Listener::handle_event      ← accept() logic
    [3] → Listener::should_close
    [4] → Listener::to_string

Connection vtable:
    [0] → Connection::get_fd
    [1] → Connection::get_events
    [2] → Connection::handle_event    ← read/write logic
    [3] → Connection::should_close
    [4] → Connection::to_string
```

a Listener instance in memory:

```
┌──────────────────────────────────────┐
│  vptr  ─────────────────────────────►│  Listener vtable
├──────────────────────────────────────┤  [Listener::get_fd, ...]
│  port  (in_port_t)                   │
├──────────────────────────────────────┤
│  fd    (Fd)                          │
└──────────────────────────────────────┘
```

a virtual call `handler->handle_event(mask)` compiles to:

```
1. load vptr from *handler
2. load function pointer from vtable[2]
3. call that function pointer with (handler, mask)
```

two indirections: one to the vtable, one to the function. this is the
runtime cost of virtual dispatch — fixed overhead regardless of class
hierarchy depth.


---


## pure virtual: = 0

```cpp
virtual void handle_event(uint32_t events) = 0;
```

`= 0` declares the method pure virtual. consequences:

    EpollHandler has no implementation of handle_event.
    EpollHandler cannot be instantiated directly.
    any concrete derived class must provide an implementation,
    or it too becomes abstract and cannot be instantiated.

this is how the interface is enforced. the compiler rejects any attempt to
instantiate EpollHandler, and rejects any derived class that fails to
implement all pure virtual methods.

`= 0` is not "null" in the pointer sense — it is a declaration syntax
meaning "no implementation here; derived classes must supply one."


---


## what each handler actually does

the two concrete handlers implement handle_event differently, reflecting
their different roles:

### Listener::handle_event

```cpp
void Listener::handle_event(uint32_t events) {
    if (!(events & EPOLLIN)) return;

    while (true) {
        int connection_fd = accept(fd.get(), ...);
        if (connection_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            throw ...;
        }
        // make non-blocking
        fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK);
        WebServ::add_connection(connection_fd);   // creates a Connection handler
    }
}
```

a listen fd becoming readable means one or more clients are waiting to
be accepted. the handler drains all pending connections in a loop (since
the fd is non-blocking, EAGAIN signals exhaustion). for each accepted fd,
it registers a new Connection handler. the Listener itself never closes
(`should_close()` returns false).

### Connection::handle_event

```cpp
void Connection::handle_event(uint32_t events) {
    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        state = ConnectionState::CLOSE; return;
    }

    if (events & EPOLLIN) {
        // drain read fd into http_parser
        // if response ready: fill write_buffer, update epoll to watch EPOLLOUT
    }

    if (events & EPOLLOUT) {
        // drain write_buffer to fd
        // if write_buffer empty: transition to CLOSE or back to READ
    }
}
```

a client fd can be readable (incoming request bytes), writable (kernel
send buffer has space), or in an error condition. the Connection handles
all three. note `update_epoll_events()` — when the Connection's interest
changes (from reading to writing), it modifies its own epoll registration
via `epoll_ctl(EPOLL_CTL_MOD)`. the loop does not do this; the handler
manages its own interest mask.


---


## why the dispatcher is type-ignorant by design

the dispatch site in run() is:

```cpp
EpollHandler *handler = ...;
handler->handle_event(events[i].events);
```

this code will not change when a new handler type is added (e.g. a CgiPipe
handler for CGI stdout). the new type implements the interface, is
registered via add_epoll_handler, and the loop dispatches to it correctly
without modification. the loop is closed to modification, open to extension
— this is the open/closed principle expressed structurally through virtual
dispatch.


---


## cost

virtual dispatch costs two pointer dereferences per call. for a network
server spending milliseconds on I/O per event, this cost is negligible.

branch prediction failure on vtable dispatch is the real concern in
performance-critical hot paths — the CPU cannot predict which function
pointer will be loaded until the vptr is dereferenced. in webserv's case,
there are at most two concrete handler types, and the CPU's branch predictor
will handle this well in practice.