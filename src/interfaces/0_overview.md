# interfaces — overview

## what lives here

```
include/interfaces/EPollHandler.hpp
src/interfaces/EPollHandler.cpp
```

2 files, 1 entity: `EpollHandler`.


---


## what EpollHandler is

an abstract base class defining the contract every event handler must
satisfy to participate in the event loop. 
concrete handlers (Listener, Connection) inherit from it.

the event loop in `WebServ::run()` holds only `EpollHandler*` pointers.
it knows nothing of Listener or Connection. EpollHandler is the boundary
at which the loop's type-ignorance becomes possible.


---


## EPollHandler.hpp

2 things in 1 file:

### 1. the abstract class

```cpp
class EpollHandler {
    virtual ~EpollHandler();
    void update_epoll_events();            // non-virtual, shared utility
    virtual int get_fd()          const = 0;
    virtual uint32_t get_events() const = 0;
    virtual void handle_event(uint32_t)  = 0;
    virtual bool should_close()   const = 0;
    virtual std::string to_string() const = 0;
};
```

pure virtual methods are the contract obligations. 
`update_epoll_events()` is shared infrastructure: 
calls `epoll_ctl(EPOLL_CTL_MOD)` using virtual `get_fd()` and `get_events()`
— used by handlers when their interest mask changes.

### 2. the WebServ namespace block

```cpp
namespace WebServ {
    extern int epfd;
    extern std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;
    void add_epoll_handler(std::unique_ptr<EpollHandler>);
    void remove_epoll_handler(int fd);
}
```

declarations (not definitions) of the server-level registry and its
operations. placed here so that every file including `EPollHandler.hpp`
— i.e. every concrete handler — gets access to `add_epoll_handler` and
`remove_epoll_handler` without an additional include. 
a convenience coupling: the abstraction header carries 
the infrastructure declarations its implementors need.


---


## EPollHandler.cpp

3 things:

```cpp
EpollHandler::~EpollHandler() {}          // virtual destructor body

void EpollHandler::update_epoll_events()  // shared epoll_ctl(MOD) logic
{ ... }

namespace WebServ {
    int epfd = -1;                         // definitions of the extern declarations
    std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;
    void add_epoll_handler(...) { ... }    // registry add: epoll_ctl(ADD) + vector
    void remove_epoll_handler(...) { ... } // registry remove: epoll_ctl(DEL) + reset
}
```

the `extern` declarations in the header become definitions here. this is
the standard C++ pattern for namespace-scoped variables shared across
translation units: declare with `extern` in the header, define once in
one `.cpp` file.

`add_epoll_handler` and `remove_epoll_handler` are the only 2 mutation
points for the handler registry. all handler lifecycle — creation,
registration, deregistration, destruction — passes through them.


---


## further reading

_meta/1_knowledge->dev/ghr/architecture/