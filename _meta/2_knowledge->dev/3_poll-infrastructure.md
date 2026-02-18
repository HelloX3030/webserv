# poll() vs epoll(): Implementation Decision

## Core difference

**poll():** You maintain array of fds. Kernel scans entire array each call.

**epoll():** Kernel maintains interest set. Returns only ready fds.
```cpp
// poll() - your state
std::vector<pollfd> fds;  // You manage this array
poll(fds.data(), fds.size(), timeout);
for (auto& pfd : fds) {  // Scan all
    if (pfd.revents) { }
}

// epoll() - kernel's state
int epfd = epoll_create1(0);  // Kernel manages interest set
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);  // Tell kernel what to watch
epoll_wait(epfd, events, MAX_EVENTS, timeout);
for (int i = 0; i < ready; i++) {  // Only ready fds
    // Process events[i]
}
```

**Consequence:** poll() complexity grows with total fds, epoll() grows with active fds.

## API comparison

| Operation | poll() | epoll() |
|-----------|--------|---------|
| Setup | Create `vector<pollfd>` | `epoll_create1()` |
| Add fd | `fds.push_back({fd, POLLIN, 0})` | `epoll_ctl(epfd, ADD, fd, &ev)` |
| Remove fd | `fds.erase(it)` | `epoll_ctl(epfd, DEL, fd, NULL)` |
| Change interest | `fds[i].events = POLLOUT` | `epoll_ctl(epfd, MOD, fd, &ev)` |
| Wait | `poll(fds.data(), n, timeout)` | `epoll_wait(epfd, events, max, timeout)` |
| Process results | Scan entire array | Iterate returned events only |

**Complexity:**
- poll(): O(n) to wait, O(n) to process (n = total fds)
- epoll(): O(1) to wait, O(k) to process (k = ready fds)

## Implementation complexity

### poll() requires:
```cpp
std::vector<pollfd> pollfds;
std::map<int, Connection*> connections;

while (true) {
    poll(pollfds.data(), pollfds.size(), -1);
    
    for (auto& pfd : pollfds) {
        if (pfd.revents & POLLIN) {
            Connection* conn = connections[pfd.fd];
            handle_read(conn);
        }
    }
}
```

State management: Array index maps to fd. Simple.

### epoll() requires:
```cpp
int epfd = epoll_create1(0);
std::map<int, Connection*> connections;

// Registration phase
for (auto& [fd, conn] : connections) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;  // Or ev.data.ptr = conn (lifetime issues)
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

// Event loop
struct epoll_event events[MAX_EVENTS];
while (true) {
    int ready = epoll_wait(epfd, events, MAX_EVENTS, -1);
    
    for (int i = 0; i < ready; i++) {
        int fd = events[i].data.fd;
        Connection* conn = connections[fd];  // Separate lookup
        handle_read(conn);
    }
}
```

State management: Must maintain separate fd→Connection map. Indirection.

Additional decisions required:
- Store `fd` or `ptr` in `epoll_event.data`?
- Level-triggered or edge-triggered? (poll() has no choice)
- How to handle `EPOLL_CTL_ADD` failures?

**Code ratio:** epoll() is ~40% more code for equivalent functionality.

## Performance scaling

**At 100 concurrent connections:**

poll(): Scan 100 fds per iteration
- Kernel scans: O(100)
- You scan: O(100)

epoll(): Scan ~5 ready fds per iteration (typical)
- Kernel scans: O(1) (maintains ready list)
- You scan: O(5)

**Difference:** Negligible. Both complete in microseconds.

**At 10,000 concurrent connections:**

poll(): Scan 10,000 fds per iteration
- Becoming measurable overhead

epoll(): Scan ~5 ready fds per iteration
- Overhead unchanged from 100-connection case

**Crossover point:** ~1000-2000 connections, depending on activity ratio.

## Decision framework

### Use poll() unless:

1. **You measured poll() as bottleneck** (profiling data required)
2. **You expect > 2000 concurrent connections** (rare for learning project)
3. **Learning epoll() is explicit goal** (educational value)

### Why poll() default for webserv:

**Bottleneck will NOT be event notification:**

| Operation | Typical latency |
|-----------|----------------|
| poll() overhead | ~10μs (100 fds) |
| epoll() overhead | ~3μs |
| **Savings:** | **~7μs** |
| | |
| Disk read | 500μs - 5ms |
| HTTP parse | 10μs - 100μs |
| Network RTT | 1ms - 100ms |

The 7μs saved is 0.1% of request handling time.

**Optimization rule:** Optimize the slowest component first.

Slow: Disk I/O, CGI execution, HTTP parsing
Fast: Event notification (whether poll or epoll)

### When to switch to epoll():

1. Profile shows event loop consuming > 5% CPU
2. Connection count exceeds 2000
3. After optimizing actual bottlenecks (disk I/O, parsing)

## Recommendation

**Start with poll().**

Reasons:
1. Simpler mental model (array-based)
2. 40% less code to write and debug
3. Adequate performance for expected load
4. POSIX portable (epoll is Linux-only)
5. Can migrate to epoll() later if profiling justifies

**Implementation order:**
1. Build with poll()
2. Profile under realistic load
3. If event loop is bottleneck (unlikely), migrate to epoll()
4. If not (likely), optimize actual bottlenecks (disk I/O, parsing)

Premature optimization wastes development time. 
Measure first, optimize second.


-------------------------------------------------------------------------------

Lukas' comments:


Habe mir gerade das ganze genauer angeschaut, muss aber sagen dass ich dir nicht ganz zustimme.

Performance Gain: Nicht wirklich relevant, true. Aber das ist auch nicht der Punkt, weshalb ich es machen wollte.
Implementation Complexity: Es gibt Grundlegend mehr Sachen die getan werden muessen, true, aber kaum. Es ist nicht +40% oder so, wie chat gpt/claude behaupted hat. So in meinen Tests, hat es eher nach +10% ausgesehen. Und diese sachen sind alle schoen seperat, unsere Event Loop, wird vermutlich sogar deutlich kuerzer, wenn wir z. b. event.data.ptr nutzen. Und es ist nicht das ganze Projekt +10%, sondern ein kleiner Teil des init codes, und der event loop...
Warum also e-poll? Nicht enorm viel mehr aufwand, aber dennoch glaube ich das wir dabei mehr lernen. Einfach weil echte server, e-poll, oder equivalente nutzen.

-------------------------------------------------------------------------------

claims (stripped to essence)

    +40% complexity estimate is wrong → closer to +10%
    Event loop gets shorter with event.data.ptr
    Real servers use epoll → therefore more to learn


1. Complexity estimate dispute

He's right to challenge the 40% figure. That number depended on what was being counted. Let's be precise.

epoll adds over poll:

    epoll_create1(0) — one line
    epoll_ctl() CRUD calls instead of vector push/erase
    epoll_wait() instead of poll()
    Event data handling choice

epoll reduces:

    With event.data.ptr: eliminates the fd → Connection* map lookup entirely. 
    You attach the pointer at registration time; epoll_wait hands you the Connection directly.

So the hot path loop is:
```cpp
// poll() hot path
for (auto& pfd : all_fds) {
    if (pfd.revents & POLLIN) {
        Connection* c = fd_map[pfd.fd]; // lookup
        handle(c);
    }
}

// epoll() hot path with data.ptr
for (int i = 0; i < n_ready; i++) {
    Connection* c = (Connection*)events[i].data.ptr; // direct
    handle(c);
}
```

Lukas is correct: the event loop body is both shorter and cleaner. 
The complexity overhead lives only in init/registration code, which is cold path. 
His +10% estimate is the more honest number.