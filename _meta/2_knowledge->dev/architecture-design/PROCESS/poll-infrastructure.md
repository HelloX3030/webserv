# poll() vs epoll(): Implementation Comparison



## API Complexity

### poll() - Array-Based Model

```cpp
struct pollfd {
    int fd;        // file descriptor
    short events;  // requested events (POLLIN, POLLOUT)
    short revents; // returned events
};

// Setup
std::vector<pollfd> fds;
fds.push_back({listen_fd, POLLIN, 0});
fds.push_back({client_fd, POLLIN, 0});

// Wait
int ready = poll(fds.data(), fds.size(), timeout_ms);

// Process - O(n) scan
for (auto& pfd : fds) {
    if (pfd.revents & POLLIN) {
        handle_read(pfd.fd);
    }
}
```

**Mental model:** "I have an array of fds I'm watching. Poll tells me which are ready."

**Operations:**
1. Populate array with fds + interest flags
2. Call poll()
3. Scan array for ready fds

**State management:** The array itself is your state. 
Index into `fds` corresponds to connection.

---


### epoll() - Kernel Interest Set Model

```cpp
// Setup
int epfd = epoll_create1(0);

struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = listen_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

ev.data.fd = client_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

// Wait - kernel returns ONLY ready fds
struct epoll_event events[MAX_EVENTS];
int ready = epoll_wait(epfd, events, MAX_EVENTS, timeout_ms);

// Process - O(1), only ready fds returned
for (int i = 0; i < ready; i++) {
    if (events[i].events & EPOLLIN) {
        int fd = events[i].data.fd;
        handle_read(fd);
    }
}
```

**Mental model:** "Kernel maintains interest set. 
I add/remove/modify interests. epoll_wait returns only ready fds."

**Operations:**
1. Create epoll instance
2. Register fds via epoll_ctl() (ADD/MOD/DEL)
3. Call epoll_wait() - returns ready events
4. Process ready events

**State management:** Kernel tracks interest set. 
You must maintain separate fd→Connection mapping.

---

## Complexity Comparison

| Aspect | poll() | epoll() |
|--------|--------|---------|
| **API calls** | 1 (poll) | 3 (create, ctl, wait) |
| **Conceptual model** | Array of interest | Kernel-side interest set |
| **Adding fd** | Append to array | epoll_ctl(ADD) |
| **Removing fd** | Erase from array | epoll_ctl(DEL) |
| **Modifying interest** | Change events field | epoll_ctl(MOD) |
| **Ready fd detection** | O(n) scan | O(1) - kernel returns ready list |
| **State coupling** | Array index = fd state | Must maintain fd→data map |
| **Lines of code** | ~20 for basic loop | ~35 for equivalent |

---

## Performance Characteristics

### poll() Performance

**Time complexity:**
- Registration: O(1) - append to vector
- Wait: O(n) - kernel scans all fds
- Result processing: O(n) - you scan all fds

**For 100 concurrent connections:**
- poll() scans 100 fds each iteration
- Modern CPU: ~5-10μs total

**For 10,000 concurrent connections:**
- poll() scans 10,000 fds each iteration
- ~500μs-1ms overhead

**Bottleneck threshold:** ~1000-2000 concurrent connections

---

### epoll() Performance

**Time complexity:**
- Registration: O(log n) - red-black tree in kernel
- Wait: O(1) - kernel maintains ready list
- Result processing: O(k) where k = number of ready fds

**For any N concurrent connections:**
- epoll_wait() returns only ready fds (typically 1-10 per iteration)
- Processing time independent of total connection count
- ~2-5μs overhead regardless of scale

**Bottleneck threshold:** Effectively none for connection count

---

## Additional Complexity in epoll()

### Data Association

poll():
```cpp
std::map<int, Connection*> connections;
std::vector<pollfd> pollfds;

// fd→Connection lookup implicit via array
for (auto& pfd : pollfds) {
    if (pfd.revents & POLLIN) {
        Connection* conn = connections[pfd.fd];
    }
}
```

epoll():
```cpp
// Option 1: Store fd, lookup separately
ev.data.fd = fd;
Connection* conn = connections[fd];

// Option 2: Store pointer directly (lifetime management critical)
ev.data.ptr = conn;
```

Must explicitly choose association strategy.

---

### Edge-Triggered vs Level-Triggered

poll(): Always level-triggered
- fd remains ready until you drain it
- Simpler mental model

epoll(): Can choose
- Level-triggered (default): Like poll()
- Edge-triggered (EPOLLET): Notified only on state change
  - More efficient but requires careful handling
  - Must read until EAGAIN
  - Easy to introduce bugs

---

## Decision Framework

### Use poll() when:
- Concurrent connections < 1000
- Simplicity is priority
- Portability matters (POSIX standard)
- Learning core event-driven model
- Avoiding premature optimization

### Use epoll() when:
- Concurrent connections > 2000
- Performance profiling shows poll() bottleneck (rare)
- Learning epoll() is educational goal
- Linux-only deployment acceptable

---

## Recommendation for webserv

**Start with poll():**

1. **Simplicity:** 30% less code, simpler mental model
2. **Adequacy:** webserv likely serves < 1000 connections
3. **Portability:** Works on all UNIX systems
4. **Evolvability:** Can swap to epoll() later if profiling shows need

**Performance comparison for typical webserv:**
- poll() overhead: ~5-10μs per iteration
- epoll() overhead: ~2-5μs per iteration
- Savings: ~3-7μs per event loop iteration

**Context:**
- Disk I/O: 500μs - 5ms per file read
- Network latency: 1ms - 100ms per request
- HTTP parsing: 10μs - 100μs

The 3-7μs saved by epoll() is noise compared to I/O costs.

---

## Implementation Effort

### Relative complexity:
poll(): 
Simpler mental model, fewer moving parts, direct correspondence between code and behavior.

epoll(): 
Additional registration phase, separate interest management, 
indirection between registration and event handling.

### Development implications:
The added complexity in epoll() translates to more code to write, 
more state to track, more edge cases to handle. 
This matters during initial development and debugging—not runtime performance.

### Critical question:
Is the performance gain worth the implementation complexity for your specific use case?

For webserv: No. 
The bottleneck will be disk I/O, HTTP parsing, and CGI execution—
not the event notification mechanism.

When epoll() complexity is justified: 
When profiling reveals the event loop itself as a bottleneck, 
which requires thousands of concurrent connections to manifest.