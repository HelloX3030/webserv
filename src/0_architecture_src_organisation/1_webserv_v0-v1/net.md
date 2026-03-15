## predicate

reactor infrastructure. sockets, epoll, connections.

EPollHandler: abstract contract for event loop participants.
Listener: accepts connections, dispatches to Connection factory.
Connection: client fd lifecycle, I/O multiplexing.

---

## naming

chosen: "net"
- networking and event handling
- accurate, accessible

rejected:

| name     | problem                                              |
|----------|------------------------------------------------------|
| reactor/ | precise but jargon-heavy                             |
| io/      | conflates network I/O and file I/O (different semantics) |

---

## architectural debt

Connection owns HttpParser. this couples net/ to http/.

in a purely layered design, net/ would be protocol-agnostic:
- Connection handles bytes only
- protocol layer interprets bytes via callback or interface

for webserv v1 (HTTP only), coupling is acceptable.

for v2 (multi-protocol):
- extract generic ByteStreamHandler interface
- http/ implements it
- Connection holds interface pointer, not concrete parser

---

## v0 → v1

previously in `classes/`. extracted as distinct layer.
