# HTTP Server Implementation: Knowledge Sources

## Information Source Taxonomy

Sources organized by authority level and purpose:

**Tier 0: Normative/Specification**
Authority: Definitive. Defines correctness.
Purpose: Legal specification of protocol/system. Highest authority.
How to use: Final arbiter of "what is correct". Verify implementation matches specification exactly.

**Tier 1: Foundational/Authoritative**
Authority: Expert consensus. Widely accepted as definitive explanations.
Purpose: Deep understanding of principles
How to use: Learn "why" things work, master fundamentals

**Tier 2: Pedagogical/Tutorial**
Authority: Teaching-focused. May simplify for clarity.
Purpose: Build intuition, provide accessible entry
How to use: First pass understanding, intuition building
before diving into specifications and expert texts.

**Tier 3: Exemplary/Reference Implementations**
Authority: Proven in production. Shows "how experts do it".
Purpose: Study design patterns, architectural decisions
How to use: See theory applied, understand trade-offs

**Tier 4: Instrumental/Tools**
Authority: Pragmatic. Enables verification.
Purpose: Test, debug, verify correctness
How to use: Validate implementation, find bugs

**Tier 5: Philosophical/Architectural**
Authority: Explanatory. Design rationale.
Purpose: Understand "why this design exists"
How to use: Grasp archetypal decisions, telos of system

---

## Tier 0

### RFC 1945: HTTP/1.0 Specification

Possible sources:
https://www.rfc-editor.org/rfc/rfc1945.html
https://datatracker.ietf.org/doc/html/rfc1945

**Priority sections:**
- Section 4: HTTP Message format (the grammar)
- Section 5: Request semantics (GET, POST, DELETE)
- Section 6: Response structure (status codes, headers)
- Section 8: Connection management

**Note**: HTTP/1.0 suggested by 42 subject as reference point. 
Simpler than HTTP/1.1.


### RFC 3875: CGI Specification
https://www.rfc-editor.org/rfc/rfc3875.html

**Covers:**
- External program execution protocol
- Environment variables (REQUEST_METHOD, QUERY_STRING, etc.)
- Request body handling
- Response format from CGI scripts


### RFC 3986: URI Generic Syntax
https://www.rfc-editor.org/rfc/rfc3986.html

**Covers:**
- Formal definition of URLs/URIs
- Parsing rules and algorithms
- Component structure: scheme, authority, path, query, fragment
- Valid character sets

---




## Tier 1


### W. Richard Stevens: Unix Network Programming Vol 1 (3rd Edition)

**Essential chapters:**

**Chapter 6: I/O Multiplexing**
- `select()` and `poll()` mechanisms
- Why non-blocking I/O is necessary (scaling proof)
- Descriptor set management
- Return value semantics

**Chapter 16: Nonblocking I/O**  
- Nonblocking socket configuration
- `fcntl()` operations (`O_NONBLOCK` flag)
- Handling `EAGAIN`/`EWOULDBLOCK`
- Edge cases: partial reads, partial writes

**Chapter 5: TCP Client/Server Example**
- `socket()`, `bind()`, `listen()`, `accept()` semantics
- Connection lifecycle
- Error handling patterns
- `SO_REUSEADDR` socket option

**Why Stevens**: Explains *why* systems designed this way.
Proves necessity from first principles. Mathematical rigor meets practical clarity.




### W. Richard Stevens: TCP/IP Illustrated Vol 1

**Essential chapters:**

**Chapter 18: TCP Connection Establishment and Termination**
- Three-way handshake (SYN, SYN-ACK, ACK)
- Why this design necessary
- Four-way termination (FIN, ACK, FIN, ACK)
- `TIME_WAIT` state (affects server restarts)

**Chapter 20: TCP Timeout and Retransmission**
- Reliable stream semantics
- Why HTTP can assume reliable delivery
- Exponential backoff algorithms

**Why this book**: Understand the layer below HTTP.
Know what TCP provides so you understand what HTTP doesn't need to handle.




### Michael Kerrisk: The Linux Programming Interface

**Essential chapters:**

**Chapter 61: Sockets: Concepts**
- Socket fundamentals
- File descriptor semantics
- Socket addresses and structures

**Chapter 63: Alternative I/O Models**
- `epoll()` (Linux-specific, highly efficient)
- Comparison with `poll()` and `select()`
- Level-triggered vs edge-triggered events

**Chapter 24: Process Creation**
- `fork()` semantics (copy-on-write)
- `exec()` family (replacing process image)
- File descriptor inheritance
- For CGI execution

**Why Kerrisk**: More modern than Stevens. Covers Linux-specific APIs.
Excellent for file descriptor and process management details.



---



## Tier 2


### Julia Evans: Technical Zines

**Author site**: https://jvns.ca/
**Zines**: https://wizardzines.com/

**Recommended zines:**
- *Networking! ACK!* - https://wizardzines.com/zines/networking/
  - TCP/IP fundamentals in comic format
  - Demystifies networking stack
  
- *HTTP: Learn your browser's language*
  - HTTP message structure
  - Common headers and their purposes
  - Visual, memorable explanations

**Why Julia Evans**: Comic format makes complex concepts memorable. 
Removes intimidation factor. Good for intuition.

Review:
[20260211]
kinda cute, helpful to see some basic images,
but doesn't speak to me. Not intuitive enough, not enough semantic depth.
Feels like dumbed down, but still dry & technical.


### HTTP Made Really Easy

https://www.jmarshall.com/easy/http/

**Content:**
- Step-by-step HTTP tutorial
- Actual telnet session examples
- Shows raw request/response pairs
- Very practical, hands-on

**Why this**: Shows HTTP in action. Bridge between theory and practice.




### MDN Web Docs (Mozilla Developer Network)

**Main resource**: https://developer.mozilla.org/en-US/docs/Web

**HTTP Documentation**: https://developer.mozilla.org/en-US/docs/Web/HTTP
- HTTP messages structure (visual diagrams)
- Request methods explained
- Status codes with use cases
- Headers reference

**Web Glossary**: https://developer.mozilla.org/en-US/docs/Glossary
- Clear definitions of terms
- Cross-referenced concepts

**Why MDN**: Clear, visual, accessible. 
Good for building mental models before formal study.




### Beej's Guide to Network Programming

Beej's Home Page - https://beej.us/
Guides - https://beej.us/guide/

https://beej.us/guide/bgnet/

**Content:**
- C socket programming tutorial
- `poll()` and non-blocking I/O explained clearly
- Practical examples with full code
- More accessible than Stevens for first encounter

**Why Beej**: Free, online, clear. Good first introduction before deep dive into Stevens.




### Computer Networking: A Top-Down Approach (Kurose & Ross)

**Relevant section:**
- Chapter 2: Application Layer
  - HTTP protocol overview
  - Client-server architecture
  - Persistent vs non-persistent connections

**Why this book**: More pedagogical than Stevens. Better diagrams. 
Good conceptual overview.



---



## Tier 3

Authority: Proven in production. Shows expert patterns.
Usage: Study architecture and design decisions. See theory applied.

### NGINX

**Creator**: Igor Sysoev
**Source**: https://github.com/nginx/nginx
**Language**: C
**Status**: Powers ~30% of web servers globally

**Study for:**
- Non-blocking I/O implementation patterns
- HTTP parsing state machines (see `src/http/ngx_http_parse.c`)
- Configuration file design (`nginx.conf` syntax)
- Event-driven architecture (`src/event/`)
- Memory pool management
- Modular design patterns

**Warning**: NGINX is production code - optimized, complex. Don't try to read everything. Focus on specific modules:
- `src/http/ngx_http_request.c` - request handling
- `src/http/ngx_http_parse.c` - parsing state machine
- `src/core/ngx_connection.c` - connection management



### Apache HTTP Server

**Source**: https://github.com/apache/httpd
**Language**: C
**Status**: The original HTTP server titan

**Study for:**
- CGI implementation (very well documented)
- Module architecture (plug-in system)
- Multi-processing models (prefork, worker, event)

**Note**: More complex than NGINX. Use selectively.



### GNU Libmicrohttpd

**Source**: https://www.gnu.org/software/libmicrohttpd/

**Why study this:**
- Small, clean HTTP server library
- Minimal necessary structure
- Clear separation of concerns
- High educational value
- GNU coding standards (readable)

**Best for**: Understanding minimal viable HTTP server architecture.




---




## Tier 4

Authority: Pragmatic. Enables empirical verification.
Usage: Test implementation, find bugs, validate correctness.

### Manual Testing Tools

**telnet** - Raw TCP connection
```bash
telnet localhost 8080
GET / HTTP/1.0
[blank line]
```
Purpose: See exact bytes sent/received. Understand protocol at lowest level.
Note: 42 subject explicitly requires testing with telnet.

**curl** - Sophisticated HTTP client
```bash
curl -v http://localhost:8080/
curl -X POST -d "data" http://localhost:8080/submit
curl -i http://localhost:8080/  # show headers
```
Purpose: Test various request types, inspect responses.

**netcat (nc)** - Swiss army knife of networking
```bash
nc -l 8080  # Listen mode
echo "GET / HTTP/1.0\r\n\r\n" | nc localhost 8080
```
Purpose: Raw socket communication. Good for testing edge cases.



### Network Analysis Tools

**tcpdump** - Packet capture (command-line)
```bash
tcpdump -i lo port 8080 -A  # ASCII output
```
Purpose: See actual TCP packets. Debug connection issues.

**Wireshark** - Packet capture (GUI)
Purpose: Visual packet inspection. Follow TCP streams. See HTTP at packet level.

**Browser Developer Tools** (Chrome/Firefox DevTools)
- Network tab: See all requests/responses
- Headers inspection
- Timing analysis
Purpose: Understand how real browsers use HTTP.



### Stress Testing Tools

**Apache Bench (ab)**
```bash
ab -n 1000 -c 10 http://localhost:8080/
```
Purpose: Load testing. Find performance bottlenecks.

**wrk** - Modern HTTP benchmarking tool
```bash
wrk -t12 -c400 -d30s http://localhost:8080/
```
Purpose: More sophisticated than ab. Scriptable.

**siege** - HTTP load tester
```bash
siege -c 100 -r 10 http://localhost:8080/
```
Purpose: Simulate multiple users. Test concurrent connections.



### Verification Strategy

Compare your server against NGINX:
1. Send identical request to both servers
2. Responses should match (status codes, headers, body)
3. Any difference = potential bug or RFC violation




---




## Tier 5

### Roy Thomas Fielding Dissertation (2000)

**Title**: *Architectural Styles and the Design of Network-based Software Architectures*

**URL**: https://roy.gbiv.com/pubs/dissertation/top.htm

**Fielding's role**: Co-author of HTTP/1.1 specification. Invented REST.

**Key chapters:**
- Chapter 5: Representational State Transfer (REST)
  - Why stateless architecture
  - Why client-server separation
  - Why uniform interface
  - Why layered system
  - Constraints and their rationale

**Why read this**: Understand HTTP's telos. Not "how" but "why". 
Design from first principles.

**When to read**: Optional given time constraints. 
Use if questioning fundamental HTTP design decisions. Otherwise, focus on implementation first.

### Tim Berners-Lee: Original Vision

**"Information Management: A Proposal"** (1989)
- Original WorldWideWeb concept
- Why hypertext matters
- The vision that birthed HTTP

**Historical context**: Understanding *why* the Web exists clarifies HTTP's purpose.