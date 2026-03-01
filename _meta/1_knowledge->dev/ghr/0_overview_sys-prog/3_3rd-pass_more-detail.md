## what problem does HTTP solve?

imagine: a person in Zürich wants to read a document stored
on a computer in Edinburgh. different machines, different OSs,
different memory spaces. how to communicate?

core problem: distributed resource access across incompatible systems.

HTTP's solution: universal protocol — shared language any computer
speaks, regardless of implementation.

like postal service rules: any post office worldwide handles a letter
if it follows standard address format. any HTTP server handles a
request if it follows HTTP message format.


## what is an HTTP server?

a translator and coordinator:

1. translates client requests (network messages) into local operations
   (read file, execute program)
2. coordinates multiple simultaneous conversations
3. responds with properly formatted messages

like a multilingual receptionist at a busy hotel:
- speaks the guest's language (HTTP protocol)
- knows the hotel layout (filesystem, resources)
- handles many guests simultaneously (concurrent connections)
- never stuck waiting for 1 guest while others need help (non-blocking)


---


## 3 core challenges


### challenge 1: multiple simultaneous clients

problem: handle 100 clients at once with 1 thread.

naive approach (blocking I/O):
```
for each client:
    read request      // might take 10s if client slow
    process
    send response
next client
```

client 1 slow → clients 2-100 wait. unacceptable.

solution (non-blocking I/O + poll):
```
loop:
    ask OS: "which clients ready to read/write?"
    for each ready client:
        do small amount of work
        if not finished, continue next iteration
    repeat
```

all clients progress concurrently. server never waits — if 1 client
not ready, serve others.

analogy: restaurant kitchen with multiple orders cooking. don't stare
at 1 pot waiting to boil — check other dishes, prep ingredients,
plate finished orders. when that pot boils, timer tells you.

`poll()` is the timer system — tells you when each client ready.


### challenge 2: partial data arrival

problem: network sends data in chunks. `read()` might return:
- full request (lucky)
- part of request (common)
- 0 bytes, data hasn't arrived (very common with non-blocking)

must remember state between reads:

```
Connection {
    socket
    state: READING_REQUEST_LINE | READING_HEADERS | READING_BODY | WRITING
    buffer: bytes received so far
    bytes_expected: from Content-Length
    bytes_received: counter
}
```

each `read()` appends to buffer. when
`bytes_received == bytes_expected`, request complete.

analogy: receiving long fax page by page. don't discard earlier pages
while waiting for rest. stack until complete document.


### challenge 3: parsing HTTP messages

HTTP has specific structure (RFC 1945):

```
GET /index.html HTTP/1.0
Host: example.com
User-Agent: Mozilla/5.0

[optional body]
```

must parse correctly:
- extract method (GET), URI (/index.html), version (HTTP/1.0)
- parse headers (name: value pairs)
- handle body if present (Content-Length tells how much)

parser as state machine:
```
START → METHOD → URI → VERSION → HEADER_NAME → HEADER_VALUE → BODY → DONE
```

each byte advances state machine. invalid input → ERROR → 400.

analogy: following assembly instructions step-by-step. each
instruction tells you what to do next. skip or reorder = broken.


---


## the event loop

entire server runs on 1 loop:

```cpp
while (server_running) {
    // ask OS: which sockets ready?
    int ready_count = poll(all_sockets, timeout);
    
    // for each ready socket:
    for (int i = 0; i < ready_count; i++) {
        if (ready_for_read) {
            read_some_bytes();
            parse_if_complete();
        }
        if (ready_for_write) {
            write_some_bytes();
            close_if_complete();
        }
    }
    
    // accept new connections if listening socket ready
    if (listening_socket_ready) {
        accept_new_client();
    }
}
```

key insight: never block. `poll()` blocks for you — tells you when
work available. when it wakes, do work quickly, back to `poll()`.

analogy: air traffic controller watching radar. planes (clients)
moving at different speeds. controller gives instructions to each
when needed, never focuses on 1 plane so long others crash.


---


## HTTP message structure

see RFC 1945, sections 4 & 5 for formal grammar.

parser must strictly follow grammar. browsers forgiving; webserv
rejects malformed requests (400 Bad Request).


## response generation

after parsing request and fetching resource, construct response:

```
HTTP/1.0 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 43\r\n
\r\n
<html><body>Hello World</body></html>
```

status codes (RFC 1945 §6):
- 200 OK: success
- 400 Bad Request: malformed request
- 403 Forbidden: access denied
- 404 Not Found: resource doesn't exist
- 500 Internal Server Error: server bug

choose correct status code, send properly formatted response.


## configuration: mapping URLs to resources

config file defines how URLs map to files/scripts:

```
server {
    listen 8080;
    server_name localhost;
    
    location / {
        root /var/www/html;
        index index.html;
    }
    
    location /cgi-bin/ {
        cgi_pass /usr/bin/python3;
    }
}
```

internally, build lookup table:

```cpp
struct LocationConfig {
    string path_prefix;       // "/cgi-bin/"
    string root_directory;    // "/var/www/html"
    vector<string> index_files;
    optional<string> cgi_handler;
};

map<string, LocationConfig> locations;
```

URL resolution: longest prefix match.

request: `GET /cgi-bin/script.py`
- matches `"/"`
- also matches `"/cgi-bin/"`
- choose `"/cgi-bin/"` (more specific)
- check if CGI handler configured
- if yes, execute script; if no, serve as static file


## CGI: external program execution

CGI (Common Gateway Interface): server executes programs that
generate responses dynamically.

how it works:

1. client requests `/cgi-bin/form.py`
2. server recognizes as CGI (from config)
3. server forks child process
4. sets environment variables:
   - `REQUEST_METHOD=POST`
   - `CONTENT_LENGTH=27`
   - `QUERY_STRING=user=john`
5. executes `/usr/bin/python3 form.py`
6. writes request body to CGI stdin
7. reads CGI stdout (HTTP headers + body)
8. sends CGI output to client

critical: CGI process runs in parallel. server uses `poll()` on pipes
(fds connecting server to CGI process) to avoid blocking.

analogy: restaurant kitchen outsourcing dessert to nearby bakery.
kitchen takes order, sends to bakery, serves other dishes while
waiting, picks up dessert when ready, delivers to customer.


---


## connection state machine

each connection moves through states:

```
    [ACCEPT]
        ↓
    [READING] ←───────┐
        ↓             │
        ├─ incomplete ┘ (wait for more data)
        │
        └─ complete
            ↓
        [PROCESSING]
            ↓
        [WRITING] ←───────┐
            ↓             │
            ├─ incomplete ┘ (wait for socket ready)
            │
            └─ complete
                ↓
            [CLOSE]
```

connection can remain in READING or WRITING for multiple `poll()`
cycles. must save state between cycles.


---


## invariants

always true:

1. every connection has exactly 1 state
2. no blocking operations in event loop
3. buffers never overflow (bounds checking)
4. file descriptors always closed (no leaks)
5. HTTP responses valid per RFC
6. 1 `poll()` controls all I/O


---


## testing


### manual

```bash
# telnet (raw TCP)
telnet localhost 8080
GET / HTTP/1.0

# curl
curl -v http://localhost:8080/
curl -X POST -d "data" http://localhost:8080/submit
```


### automated

```bash
# stress test
ab -n 1000 -c 10 http://localhost:8080/

# compare against NGINX
# webserv should produce same responses for same requests
```


### edge cases

- request with no headers
- request with very large body
- client disconnects mid-request
- multiple requests on same connection (persistent connections)
- malformed requests (missing CRLF, invalid method, etc.)