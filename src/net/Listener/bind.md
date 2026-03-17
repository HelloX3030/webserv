# binding — a socket to an address

## ontology

fixing an abstract entity to a concrete referent.
in mathematics: establishing a fixed association between 2 things.
here: between a file descriptor (abstract communication endpoint)
and a network address (host:port).

before binding: socket exists as anonymous file descriptor —
connected to nothing, receiving nothing.
after binding: OS knows packets arriving at `host:port` belong to this socket.

the term is consistent across computer science:
variable binding in lambda calculus, name binding in PLs,
key binding in editors...

---

## the POSIX call

```c
int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
```

registers `sockfd` as owner of `addr` in the OS socket table.

sequence — invariant:

```
socket() → bind() → listen() → accept() → [read/write] → close()
```

`listen()` — marks socket as passive, ready to accept connections.
`accept()` — blocks until client connects, returns new socket for
that specific connection.

---

## `0.0.0.0` — the wildcard address

```
0.0.0.0:8080   — accept on port 8080 on any network interface
127.0.0.1:8080 — loopback only (same machine)
<specific IP>  — that interface only
```

assigned by the config parser when operator writes a bare port
(`listen 8080;`) without specifying a host.

---

## privilege boundary

binding to [1, 1023] requires root on POSIX — kernel enforces at
`bind()` time, not parse time.
`listen 80;` passes validation, fails at `bind()` if unprivileged:
`errno = EACCES`.

correct layering: validator checks protocol validity;
OS checks process permissions.

---

## culture

`address already in use` (`EADDRINUSE`) — a previous  still holds the bind
— is nearly universal initiation for networked programming.
`SO_REUSEADDR` is the remedy.

what's listening on port X?
`ss -tlnp` (Linux) or `lsof -i :X` (macOS).
