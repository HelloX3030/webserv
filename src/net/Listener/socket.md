# sockets

## ontology

a handle to a communication endpoint.

not "like a file" — different problem.
files: persistent, local, named by path.
sockets: transient, network-capable, named by address.

the abstraction resolves 3 fundamental problems simultaneously:
1. naming — how do you address a process on a remote machine?
2. transport — how do bytes cross address-space / machine boundaries?
3. multiplexing — how do N conversations share 1 machine?

before BSD sockets (1983): each protocol (TCP, UDP, XNS, DECnet...)
had its own syscalls. chaos. Bill Joy and colleagues at Berkeley unified them:
1 interface, N protocols underneath. polymorphism at the kernel level.

formal structure:
```
Socket := (AddressFamily, Type, Protocol, State)
Channel := Socket_a ⊗ Transport ⊗ Socket_b
```

---

## types — what they mean

SOCK_STREAM (TCP)
reliable ordered byte stream. connection-oriented.
correctness over speed. HTTP, SSH, databases. webserv uses this.

SOCK_DGRAM (UDP)
unreliable unordered datagrams. connectionless.
speed over reliability. DNS, streaming, real-time games.

SOCK_RAW (IP)
direct network layer. bypass transport entirely.
when existing transports are insufficient.
used in ping, traceroute, custom protocol research.

TCP: reliability at the cost of latency.
UDP: speed at the cost of loss.
raw: control at the cost of complexity.

---

## address structure

```c
struct sockaddr_in {
    sa_family_t    sin_family; // AF_INET
    in_port_t      sin_port;   // htons(8080)
    struct in_addr sin_addr;   // INADDR_ANY
};
```

this is the naming schema for the network:
`(address_family, port, ip)` = the full coordinates of an endpoint.

the UPPERCASE constants — `AF_INET`, `INADDR_ANY`, `SO_REUSEADDR`
— are C preprocessor macros, integer literals with semantic names,
defined in system headers shared between your process and the kernel.

`htons()` — host-to-network short.
network byte order is big-endian. CPU may be little-endian.
this is not optional: mismatched endianness means wrong port,
wrong address. conversion is necessary.

---

## server lifecycle — necessary order

```
socket()    → anonymous endpoint. just an fd.
bind()      → fix fd to (ip, port). now locatable.
listen()    → mark as passive. kernel queues incoming connections.
accept()    → dequeue 1 connection. returns new fd for that client.
recv/send() → data exchange.
close()     → release fd.
```

each step logically required by its successor.

`accept()` returns a *new* fd for each client.
the original listening socket stays open.
this is how 1 server handles N clients: the listening socket
is a factory; accept() materialises new communication channels.

---

## EADDRINUSE and SO_REUSEADDR

`EADDRINUSE`: bind() fails because the port is already claimed.
in TCP, when a socket closes, the OS holds the address in TIME_WAIT for ~2 mins
— ensuring stray packets from the old session don't corrupt a new one.
this is correct TCP behaviour.

consequence during development: kill webserv, restart immediately
→ bind() returns EADDRINUSE. the ghost of the previous process
still holds the address.


`SO_REUSEADDR`: a socket option, set before bind(), that tells the kernel:
allow rebinding to an address in TIME_WAIT.

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

mandatory for any server you intend to restart during development
— and for production resilience. this goes in the socket setup
phase, immediately after socket(), before bind().

`SOL_SOCKET` — socket level (as opposed to TCP level, IP level).
`SO_REUSEADDR` — the option name. both are macros: integers.

---

## relevant for webserv

- use SOCK_STREAM (TCP)
- set SO_REUSEADDR before bind()
- set O_NONBLOCK (via fcntl) after socket() — required for
  non-blocking I/O in the poll() event loop
- bind to 0.0.0.0 (INADDR_ANY) when config specifies bare port:
  accept on any interface
- bind to specific IP when config specifies host:port

sequence in webserv socket setup:
```
socket() → setsockopt(SO_REUSEADDR) → fcntl(O_NONBLOCK)
         → bind() → listen() → register with poll/epoll
```

---

## why not everything-is-a-file?

Plan 9 proved unification is possible:
```
/net/tcp/clone   → reserve connection
/net/tcp/N/data  → read/write
/net/tcp/N/ctl   → control
```
same open/read/write for network and files.

the BSD socket API is not the only coherent
design — it is the historical winner.
