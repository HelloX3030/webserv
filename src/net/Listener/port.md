# ports — range [1, 65535]

## ontology

16-bit unsigned integer in the TCP and UDP packet header.
identifies a specific endpoint within a host — the mechanism
by which the OS demultiplexes incoming packets to the correct process.

without ports: OS knows a packet arrived at this machine (via IP),
but not which process should receive it.
with ports: `bind(socket_fd, host:port)` registers ownership —
OS delivers packets arriving at that port to that socket.

---

## why 65535

16-bit unsigned integer: 2^16 = 65536 values, range [0, 65535].
not an OS constraint — a protocol constraint.
the port field in the TCP (RFC 793, 1981) and UDP (RFC 768, 1980) headers
is defined as 16 bits, baked into the packet format at the wire level.
every conformant TCP/IP implementation on every OS uses this field.

port 0 is reserved: binding to 0 signals the OS to assign an
available ephemeral port. not a usable server port. ∴ server-usable range begins at 1.

---

## 3 categories

put forth by the `Internet Assigned Numbers Authority (IANA)`

```
[0,     1023] — well-known.  IANA-assigned. require root on POSIX.
[1024, 49151] — registered.  IANA registry, no privilege required.
[49152, 65535] — ephemeral.  OS-assigned for outbound connections.
```

the privilege boundary at 1024 is POSIX kernel policy, not protocol.
TCP/IP has no concept of privilege — it is a flat 16-bit space.
intent: prevent unprivileged processes from impersonating
well-known services (HTTP:80, HTTPS:443, SSH:22).

the validator accepts [1, 65535] — protocol validity only.
a config with `listen 80` passes validation and fails at `bind()`
if the process lacks root (`errno = EACCES`).
correct separation: validator checks what is legal in the protocol;
the OS checks what is permitted for this process.

---

## historical flow, culture

8080 is the canonical unprivileged HTTP alternative to 80 —
not IANA-assigned, but universally understood in developer culture.

portscanning — probing a port range to discover running services —
1 of the oldest network reconnaissance techniques.
nmap (1997, Fyodor) is the canonical tool. a full [1, 65535] scan
reveals the entire service footprint of a host.

---

## imagery:
"port" = a point of entry. a server is a harbour; ports the docks.
the early internet was deliberately cast in spatial, navigational terms
— surfing, navigating, ports, addresses — to make an abstract system graspable.
