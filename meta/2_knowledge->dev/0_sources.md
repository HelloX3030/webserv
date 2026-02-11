## primary

### RFC 1945: HTTP/1.0 specification

Sources:

    https://www.rfc-editor.org/rfc/rfc1945

    https://roy.gbiv.com/protocols/http/rfc1945.html

    https://datatracker.ietf.org/doc/html/rfc1945#autoid-21


Focus areas now:

    Section 4: HTTP Message - priority
    Section 5: Request - GET/POST/DELETE semantics
    Section 6: Response - status codes
    Section 8: Connections - persistent vs non-persistent


### RFC 3875: CGI specification

https://www.rfc-editor.org/rfc/rfc3875.html

    Defines external program execution protocol
    Environment variables for server-CGI communication
    Request handling (including chunked requests)
    Response processing


### RFC 3986: URI Generic Syntax

https://www.rfc-editor.org/rfc/rfc3986.html

    Defines what URLs/URIs actually are
    Parsing rules
    Component structure (scheme, authority, path, query, fragment)



## foundational literature : systems programming

### W. Richard Stevens

**Unix Network Programming Vol 1**

Chapter 6: I/O Multiplexing

  select() and poll() mechanisms
  Why non-blocking required
  Scaling analysis (proves necessity)

Chapter 16: Nonblocking I/O

  Nonblocking sockets
  fcntl() operations
  Edge cases in nonblocking reads/writes

Chapter 5: TCP Sockets

  socket(), bind(), listen(), accept() semantics
  Connection lifecycle
  Error handling patterns


**TCP/IP Illustrated Vol 1**

Chapter 18: TCP Connection Establishment and Termination

  Three-way handshake (why it exists)
  Four-way termination
  TIME_WAIT state (affects server restarts)

Chapter 20: TCP Timeout and Retransmission

  Why HTTP can assume reliable delivery



### Michael Kerrisk 
**The Linux Programming Interface**

Chapter 61: Sockets

  Socket fundamentals
  File descriptor semantics

Chapter 63: Alternative I/O Models

  epoll
  Comparison with poll()/select()

Chapter 24: Process Creation

  fork()/exec() for CGI execution



## reference implementations

### NGINX

Creator: Igor Sysoev

Source: https://github.com/nginx/nginx
Language: C
Known for: Performance, clean architecture, elegant configuration

Study for:
- Non-blocking I/O implementation patterns
- HTTP parsing state machines
- Configuration file design
- Event-driven architecture


### GNU Libmicrohttpd

Source: https://www.gnu.org/software/libmicrohttpd/

Small, clean HTTP server library

Good for understanding minimal necessary structure
Clear separation of concerns
Educational value high



## testing & verification

telnet - manual HTTP conversation (subject explicitly requires)
curl - sophisticated HTTP client, testing requests
netcat (nc) - raw socket communication
tcpdump/wireshark - packet inspection
Browser developer tools - real request/response analysis

Stress testing:
  ab (Apache Bench)
  wrk - modern HTTP benchmarking
  siege - load testing

Compare behavior against NGINX for correctness verification



## optional : protocol design philosophy

Roy Thomas Fielding
Architectural Styles and the Design of Network-based Software Architectures (2000)

  Explains HTTP design principles from first principles
  WHY stateless, WHY client-server, WHY uniform interface
  Defines REST architectural style