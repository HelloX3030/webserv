# event loop

## what the event loop is

the event loop is the concrete instantiation of the reactor pattern's
dispatcher. where the reactor-pattern document describes structure in the
abstract, this document describes what actually exists in webserv: the
precise sequence of phases, the epoll_wait mechanics, and the real hazards
present in the running code.


---


## lifecycle: three phases

the complete lifecycle of the server, as orchestrated by main.cpp:

```
parse → init → [run] → quit
```

```cpp
// main.cpp
signal(SIGINT, handle_sigint);   // signal disposition before anything else

WebServ::parse(argc, argv);      // config frontend: produces ServerConfig[]
WebServ::init();                 // creates epoll fd, registers listeners
WebServ::run();                  // blocks here until g_running == 0
WebServ::quit();                 // destroys all handlers, closes all fds
```

these phases are necessarily ordered. each phase's output is the next
phase's precondition:
    parse produces configuration data
    init requires configuration data to register listeners; produces epfd
    run requires epfd and registered handlers
    quit requires the same registry run has been using


### phase 1: parse

`WebServ::parse()` runs the config frontend. produces the ServerConfig data
that determines which ports to listen on, routing rules, etc. no I/O
infrastructure exists yet. pure data production.


### phase 2: init

```cpp
// WebServ_init.cpp
epfd = epoll_create1(0);
```

`epoll_create1(0)` asks the kernel to create an epoll instance and returns
a file descriptor representing it. this fd is the handle through which all
subsequent `epoll_ctl` and `epoll_wait` calls operate.

after init, listeners are registered — one per configured port. each
`add_listener()` call:
    constructs a Listener (binds and listens on the port)
    calls add_epoll_handler(), which calls epoll_ctl(EPOLL_CTL_ADD)
    places the Listener in epoll_handlers[fd]

at the end of init, the epoll instance watches a set of listen fds. no
client fds exist yet.


### phase 3: run

the loop. examined in full below.


### phase 4: quit

```cpp
// WebServ_quit.cpp
for (std::size_t fd = 0; fd < epoll_handlers.size(); fd++) {
    if (epoll_handlers[fd])
        remove_epoll_handler(fd);
}
```

iterates the full registry, calling `remove_epoll_handler` on each live
entry. `remove_epoll_handler` calls `epoll_ctl(EPOLL_CTL_DEL)` then
`epoll_handlers[fd].reset()` — which destroys the unique_ptr, which
destroys the handler, whose destructor closes the fd via RAII.

the epoll fd itself (epfd) is not explicitly closed in the visible quit code.
this is an open question — see below.


---


## the loop: epoll_wait mechanics

```cpp
// WebServ_run.cpp
struct epoll_event events[EPOLL_MAX_EVENTS];   // stack-allocated, 64 slots

while (g_running) {
    int n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, EPOLL_TIMEOUT);
    // EPOLL_TIMEOUT = -1 → block indefinitely until ≥1 fd ready

    if (n < 0) {
        if (errno == EINTR) continue;           // signal interrupted — retry
        throw std::system_error(...);           // real error
    }

    for (int i = 0; i < n; ++i) {
        EpollHandler *handler =
            static_cast<EpollHandler *>(events[i].data.ptr);

        handler->handle_event(events[i].events);

        if (handler->should_close())
            remove_epoll_handler(handler->get_fd());
    }
}
```

### epoll_wait

```c
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

    epfd       — the epoll instance fd
    events     — output array; kernel writes ready events here
    maxevents  — capacity of the array; at most this many events returned
    timeout    — -1: block indefinitely. 0: return immediately. N>0: ms timeout.

the call suspends the thread in the kernel. the kernel's internal data
structure for epfd tracks all registered fds and their interest masks.
when any registered fd satisfies any watched condition (EPOLLIN, EPOLLOUT,
etc.), the kernel wakes the thread, populates the events array with up to
maxevents entries, and returns the count.

the events array is stack-allocated with fixed capacity 64. if more than 64
fds are ready simultaneously, the remainder are not lost — they remain ready
and will be returned in the next epoll_wait call. no events are discarded.

### epoll_event structure

```c
struct epoll_event {
    uint32_t events;    // which conditions are ready (EPOLLIN, EPOLLOUT, ...)
    epoll_data_t data;  // user data — we use data.ptr
};
```

`data.ptr` is what was stored at registration time:

```cpp
ev.data.ptr = static_cast<void *>(new_epoll_handler.get());
```

the kernel preserves this pointer and returns it with the event. this is
how the dispatcher recovers the handler without a registry lookup.

### EINTR handling

`epoll_wait` returns -1 with errno == EINTR when interrupted by a signal
before any fd became ready. this is not an error — it is the normal
consequence of signal delivery. the correct response is to retry.

SIGINT triggers `handle_sigint`, which sets `g_running = 0`. the loop's
next iteration checks `g_running` before calling `epoll_wait` again.
the sequence on Ctrl-C:

```
epoll_wait blocking...
↓ SIGINT delivered
handle_sigint() executes: g_running = 0; (or _Exit on second signal)
epoll_wait returns -1, errno = EINTR
continue → loop condition: while (g_running) → false
→ exit loop → quit()
```

`g_running` is `volatile sig_atomic_t` — the volatile qualifier prevents
the compiler from caching the value in a register across the loop boundary.
`sig_atomic_t` guarantees the write in the signal handler is atomic with
respect to the read in the loop.

### dispatch

```cpp
EpollHandler *handler = static_cast<EpollHandler *>(events[i].data.ptr);
handler->handle_event(events[i].events);
```

`static_cast<EpollHandler *>` recovers the typed pointer from the void*.
this is safe because the only thing ever stored in data.ptr is an
EpollHandler* (established at registration). the cast is unchecked —
correctness depends on the registration invariant holding.

`handle_event(events[i].events)` passes the bitmask of ready conditions.
the handler inspects it (EPOLLIN? EPOLLOUT? EPOLLERR?) and acts accordingly.

### deregistration within the dispatch loop

```cpp
if (handler->should_close())
    remove_epoll_handler(handler->get_fd());
```

`remove_epoll_handler` calls `epoll_ctl(EPOLL_CTL_DEL)` and then
`epoll_handlers[fd].reset()`, which destroys the handler and closes the fd.

the pointer `handler` is now dangling. the code does not dereference it
after this point — the loop increments `i` and moves to the next event.

**the hazard:** if the same fd appears more than once in the current
`events` batch (which should not happen with epoll's default level-triggered
mode for a single fd, but could occur in edge cases or with multiple fds
that happen to point to the same handler), the second iteration would
dereference a dangling pointer after the first caused removal.

in practice, a single fd produces at most one event entry per epoll_wait
call, so this hazard does not manifest. but the code structure does not
enforce this — it relies on the epoll invariant implicitly.

see open questions.


---


## timeout = -1: why block indefinitely

`EPOLL_TIMEOUT = -1` means epoll_wait blocks until at least one fd is ready.
it never returns spuriously. this is correct for a server with no timer-based
work to do between events.

if the server needed to implement connection timeouts (close idle connections
after N seconds), a positive timeout would be necessary — the loop would
wake periodically to scan for expired connections even when no fd is ready.

the current code has no timeout mechanism. this is a known gap — see open
questions.


---


## what the loop does not do

the loop is minimal. it does not:
    - track time or implement timeouts
    - handle CGI process reaping (SIGCHLD, waitpid)
    - rate-limit or prioritise among handlers
    - batch-coalesce writes

some of these are out of scope for webserv entirely. others may be
partially implemented in handlers. the loop itself is intentionally minimal.


---


## open questions

### epfd not explicitly closed in quit()

`WebServ::quit()` closes all handler fds via remove_epoll_handler, but
the epoll fd (epfd) itself is not visibly closed. the OS closes all fds on
process exit, so this is not a resource leak in practice. but a clean
shutdown would close epfd explicitly. research: is there code elsewhere
that closes epfd, or is this an acknowledged omission?

### dangling pointer safety in dispatch loop

the dispatch loop calls `remove_epoll_handler(handler->get_fd())` mid-loop
for handlers that signal should_close(). after this call, `handler` is a
dangling pointer. the code does not dereference it again in the current
iteration, but there is no structural guarantee preventing a future code
change from doing so. additionally: if two events in the same batch refer
to fds whose handlers destroy each other (e.g. a Listener creating and
immediately closing a Connection), is there a cross-handler dangling hazard?

research: what is the correct idiom for safe mid-loop deregistration in
reactor implementations? (deferred removal lists, tombstone flags, etc.)

### connection timeouts

EPOLL_TIMEOUT = -1 means idle connections are never reaped. HTTP/1.1
persistent connections that go silent will hold their fd and registry slot
indefinitely. timeout implementation requires either: a positive epoll
timeout with periodic scan, or a separate timer fd (timerfd_create on Linux)
registered as an epoll handler.