# errno inspection after I/O — forbidden

## rule

`errno` inspection after `recv()`/`send()` to adjust behaviour is prohibited.

no conditional logic branching on `EAGAIN`, `EWOULDBLOCK`, or any other
errno value after I/O syscalls.

---

## logical necessity

### premise 1 — event loop correctness

a correct event loop only invokes I/O on fds that `poll()`/`epoll_wait()`
has signalled as ready.

`recv()` is called only when POLLIN was set in revents.
`send()` is called only when POLLOUT was set in revents.

### premise 2 — EAGAIN semantics

EAGAIN means: "fd has no data available (for read) or cannot accept data
(for write) right now — try again later."

### consequence

if the event loop is correct, EAGAIN cannot occur.

the kernel signalled readiness → the fd is ready → I/O will succeed.
EAGAIN's presence proves the event loop is broken:
you called I/O without a readiness signal.

### therefore

errno inspection for EAGAIN after I/O is either:
1. redundant — if event loop is correct, condition never triggers.
2. evidence of a bug — if it triggers, the event loop is incorrect.

in neither case is it useful. in the second case, it masks a structural
defect with local workaround logic.

---

## deeper pathology

errno-based retry logic constructs a shadow state machine.

the event loop is the single arbiter of "when to act on which fd."
it maintains readiness state & decides dispatch.

errno branching duplicates this decision-making:
```cpp
// anti-pattern
ssize_t n = recv(fd, buf, len, 0);
if (n < 0 && errno == EAGAIN) {
    // re-register for poll? retry later? buffer somewhere?
    // now you have 2 control structures deciding when to I/O
}
```

this scatters fd-lifecycle logic across call sites.
the event loop can no longer be reasoned about in isolation.
behaviour depends on what individual `recv()`/`send()` wrappers do.

architecture degrades into ad-hoc coordination.

---

## errno unreliability

`errno` is a global variable (thread-local in modern libc).

any syscall between the failing call and your check can overwrite it:
```cpp
ssize_t n = recv(fd, buf, len, 0);
log_something();          // might call write() internally
if (errno == EAGAIN) {    // errno now reflects log_something()'s syscalls
    // ...
}
```

relying on errno across intervening operations is structurally fragile.
even within a single expression, sequencing can introduce intervening calls.

---

## connection to IO-multiplexing.md

the poll/epoll model's purpose is to eliminate speculation about
fd readiness. you ask the kernel, the kernel tells you. 
you act only on confirmed readiness.