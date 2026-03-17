# I/O multiplexing & polling

## the problem

a single process must serve N clients concurrently.
each client connection is a file descriptor.
any fd may or may not have data at any moment.

blocking I/O on 1 fd stalls all others.
busy-waiting burns CPU with no useful work.

the solution must be: I/O multiplexing: sleep until something is ready,
then wake and act only on what is ready.

---

## why polling — logical necessity

userspace cannot receive hardware interrupts directly.
the kernel holds all I/O state: socket buffers, pipe contents, fd readiness.
userspace must ask the kernel.

the kernel cannot call into userspace.
it can only respond to syscalls.

therefore: the application must issue 1 syscall listing all fds it cares about,
the kernel checks their readiness, and returns.
that is polling — the only possible design
given the protection boundary between userspace and kernel.

---

## alternatives and why they fail

thread per connection:
    each thread blocks on its own fd. simple.
    cost: ~8MB stack per thread. 10,000 connections = 80GB just for stacks.
    context-switching overhead compounds this. fails at scale.

busy-wait:
    loop over all fds, call non-blocking read, check EAGAIN, repeat.
    100% CPU. no sleep/wake. starves other processes. infeasible.

async I/O (POSIX aio):
    kernel performs I/O asynchronously, notifies on completion.
    on Linux: only works for O_DIRECT files, not sockets. broken for webserv usage.
        why? Linux's POSIX aio implementation only functions correctly
        with O_DIRECT file I/O — a mode that bypasses the page cache entirely.
        Socket buffers are not O_DIRECT.
        The kernel never implemented aio properly for sockets on Linux - design gap.

signal-driven I/O:
    kernel sends SIGIO on readiness. handler cannot know which fd triggered it.
    signals can be lost. race conditions. fundamentally broken for multi-fd.

---

## the mechanisms

### select (BSD 4.2, 1983)

fd_set: a bitmask. fixed at FD_SETSIZE, typically 1024.
kernel scans every bit on every call — O(n).
must rebuild the set each call. obsolete.

### poll (SVR3, 1986)

array of {fd, events, revents} structs. no fd limit.
kernel still scans every entry — O(n).
POSIX standard.

    pollfd's 2 event fields:
    `events` — what you ask the kernel to watch for
    `revents` — what the kernel writes back — what actually occurred
    "r" = returned.
    After poll() returns, you inspect revents on each fd to know what happened.

### epoll (Linux 2.5.44, 2002)

persistent interest set stored in kernel.
kernel maintains its own readiness queue.
returns only ready fds — O(1) for detection, O(ready) for results.
Linux only.

### kqueue (FreeBSD 4.1, 2000)

same O(1) design as epoll. more general:
monitors fds, signals, timers, processes — unified event type.
macOS, BSD.


development: refinement for scale.
same logical operation — ask kernel which fds are ready.
select and poll scan everything each call.
epoll and kqueue maintain state between calls.

---

## poll() — mechanical operation

```c
struct pollfd fds[] = {
    { listen_fd,  POLLIN,  0 },
    { client_fd1, POLLIN,  0 },
    { client_fd2, POLLIN | POLLOUT, 0 },
};
int n = poll(fds, 3, timeout_ms);
```

on call:
1. kernel receives array + count + timeout.
2. for each fd: queries the relevant driver's poll function —
   socket checks receive buffer; pipe checks data availability.
3. if any fd ready: sets `revents`, returns count of ready fds immediately.
4. if none ready: registers the process on each fd's wait queue, sleeps.
5. when any fd becomes ready (interrupt → kernel → buffer filled):
   kernel wakes the process.
6. process inspects `revents` to know which fds to act on.

the wait queues are the mechanism connecting hardware interrupts to process wakeup.
the process does not poll in a loop — it genuinely sleeps and is woken by the kernel.


---


## in webserv

the event loop — same logic, 2 implementations were considered:

### poll

```c
// every fd in 1 array, rebuilt or maintained each iteration
struct pollfd fds[MAX_FDS];
// register: fds[i] = { fd, POLLIN, 0 };

while (running) {
    int n = poll(fds, nfds, timeout_ms);
    for (int i = 0; i < nfds; i++) {
        if (fds[i].revents & POLLIN) {
            if (fds[i].fd == listen_fd)  accept();    // new client
            else                         recv();       // client data
        }
        if (fds[i].revents & POLLOUT)    send();       // response ready
    }
}
```

scans all fds every call regardless of how many are ready.

### epoll (Lukas's choice)

```c
// interest set lives in kernel — registered once, not rebuilt
int epfd = epoll_create1(0);

// register a fd:
struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd };
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

// add client on accept, remove on close:
epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
epoll_ctl(epfd, EPOLL_CTL_DEL, closed_fd, NULL);

while (running) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, timeout_ms);
    for (int i = 0; i < n; i++) {          // only ready fds returned
        if (events[i].data.fd == listen_fd) accept();
        else if (events[i].events & EPOLLIN) recv();
        else if (events[i].events & EPOLLOUT) send();
    }
}
```

`epoll_wait` returns only ready fds — no scanning.
at 100 connections with 5 active: `poll` scans 100, `epoll` returns 5.
the gap widens with connection count.

in both cases: 1 call per loop iteration, no blocking on any individual fd,
no `EAGAIN` handling needed — readiness guaranteed before `recv()`/`send()`.


https://nginx.org/en/docs/events.html
Connection processing methods
