# networking layer

## contains

EPollHandler — abstract contract for event loop participants
Listener — accepts connections, dispatches to Connection factory
Connection — client fd lifecycle, owns HttpParser instance, socket primitives, I/O multiplexing

## naming

### chosen: net

These are the reactor implementation. They're networking and event handling.
he name net/ is accurate but incomplete. reactor/ would be more precise but jargon-heavy. Keep net/ — it's close enough.

### others considered

also considered io/

io/ conflates:
Network I/O (sockets, poll/select/epoll)
File I/O (reading static files to serve)

These have different semantics. Socket reads can block/partial-read;
file reads (on local fs) behave differently. If I later need file utilities, they'd live in base/ or a thin fs/.



## coupling question

net/ contains Connection, which owns HttpParser.
This couples net/ to http/.

In a purely layered design, net/ would be protocol-agnostic.
For webserv's scope (HTTP only), this coupling is acceptable.

If extending to other protocols, refactor:
  - Extract generic ByteStreamHandler interface
  - Let http/ implement it
  - Connection holds interface pointer, not concrete parser
