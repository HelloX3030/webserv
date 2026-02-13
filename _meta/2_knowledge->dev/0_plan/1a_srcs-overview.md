# HTTP Server Implementation: Knowledge Sources

## Tier 0

RFC 1945: HTTP/1.0 Specification

RFC 3875: CGI Specification

RFC 3986: URI Generic Syntax

## Tier 1

Unix Network Programming
Vol 1
W. Richard Stevens
3rd edition

TCP/IP Illustrated
Vol 1
W. Richard Stevens

The Linux Programming Interface
Michael Kerrisk


OTHERS - POTENTIAL - TO PROCESS

    Internetworking with TCP/IP
    Douglas E. Comer
    Published by Pearson

    TCP/IP Network Administration
    Craig Hunt
    Published by O’Reilly & Associates, Inc

    Advanced Programming in the UNIX Environment
    W. Richard Stevens
    Published by Addison Wesley

    SSH, The Secure Shell: The Definitive Guide
    Daniel Barrett, Richard Silverman, Robert Byrnes


## Tier 2

Beej's Guide to Network Programming

Computer Networking: A Top-Down Approach
Jim Kurose, Keith Ross
    see: Jim Kurose







### Julia Evans: Technical Zines

**Author site**: https://jvns.ca/
**Zines**: https://wizardzines.com/

**Recommended zines:**
- *Networking! ACK!* - https://wizardzines.com/zines/networking/
  - TCP/IP fundamentals in comic format
  - Demystifies networking stack

Review:
[20260211]
kinda cute, helpful to see some basic images,
but doesn't speak to me. Not intuitive enough, not enough semantic depth.
Feels like dumbed down, but still dry & technical.

- *HTTP: Learn your browser's language*
  - HTTP message structure
  - Common headers and their purposes
  - Visual, memorable explanations

**Why Julia Evans**: Comic format makes complex concepts memorable. 
Removes intimidation factor. Good for intuition.


### HTTP Made Really Easy

https://www.jmarshall.com/easy/http/

**Content:**
- Step-by-step HTTP tutorial
- Actual telnet session examples
- Shows raw request/response pairs
- Very practical, hands-on

**Why this**: Shows HTTP in action. Bridge between theory and practice.



Mozilla Developer Network (MDN) Web Docs: HTTP


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