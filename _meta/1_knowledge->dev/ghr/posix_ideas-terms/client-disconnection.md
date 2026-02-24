# client disconnection handling

## specification

server must handle client disconnections gracefully.

---

## detection

see: tcp-connection-termination.md for TCP-level explanation.

### on read

```cpp
ssize_t n = recv(fd, buf, len, 0);
```

- `n == 0`: client sent FIN. clean close. connection terminated.
- `n == -1`, errno ECONNRESET: client sent RST. abrupt close.

### on write

```cpp
ssize_t n = send(fd, buf, len, 0);
```

- `n == -1`, errno EPIPE: writing to closed connection.
- `n == -1`, errno ECONNRESET: connection reset by peer.

SIGPIPE: by default, writing to closed socket sends SIGPIPE → process terminates.
must ignore SIGPIPE or use MSG_NOSIGNAL flag on send().

---

## response

on any disconnection signal:
1. close fd
2. free connection state (buffers, parsing context)
3. remove from poll/epoll interest set
4. continue event loop

no error message to client (client is gone).
log if desired.

---

## mid-operation disconnection

client may disconnect at any point:
- during request receipt (partial request in buffer)
- during response transmission (partial response sent)
- during CGI execution (response never delivered)

all cases: same handling.
clean up, move on.

partial state in buffers is discarded.
no attempt to "complete" the request.

---

## implementation pattern

```cpp
ssize_t n = recv(conn->fd, buf, len, 0);
if (n == 0) {
    close_connection(conn);
    return;
}
if (n < 0) {
    if (errno == EINTR) {
        return;  // interrupted by signal, retry on next poll cycle
    }
    close_connection(conn);
    return;
}
// n > 0: process data
```

EINTR: syscall interrupted by signal. connection still valid.
don't close — event loop will re-poll, recv will be called again.

other errno values: log if desired. control flow is always: close.

same pattern for send().

note: SIGPIPE must be ignored or MSG_NOSIGNAL used on send().
otherwise write to closed socket terminates process.