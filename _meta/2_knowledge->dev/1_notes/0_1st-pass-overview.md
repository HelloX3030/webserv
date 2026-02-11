# HTTP Server: First Pass Understanding

## What Problem Does HTTP Solve?

Imagine: A person in Boston wants to read a document stored on a computer in Berlin.
Different machines, different operating systems, different memory spaces. How do they communicate?

**The core problem**: Distributed resource access across incompatible systems.

**HTTP's solution**: A universal protocol - a shared language that any computer can speak,
regardless of implementation details.


Like postal service rules: 
Any post office worldwide can handle a letter if it follows standard address format. 
Similarly, any HTTP server can handle a request if it follows the HTTP message format.

Webserv implements this universal protocol in C++.


## What IS an HTTP Server?

At its essence, an HTTP server is a **translator and coordinator**:

1. **It translates** client requests (network messages) into local operations (read file, execute program)
2. **It coordinates** multiple simultaneous conversations with different clients
3. **It responds** with properly formatted messages the client understands

Like a multilingual receptionist at a busy hotel:
- Speaks the guest's language (HTTP protocol)
- Knows the hotel layout (filesystem, resources)
- Handles many guests simultaneously (concurrent connections)
- Never gets stuck waiting for one guest while others need help (non-blocking I/O)



## The 3 Core Challenges


### Challenge 1: Multiple Simultaneous Clients

**Problem**: Your server must handle 100 clients at once, but you have only 1 thread.

**Naive approach** (blocking I/O):
```
for each client:
    read their request    // might take 10 seconds if client is slow!
    process it
    send response
next client
```

If client 1 is slow (bad network, old computer), clients 2-100 wait. Unacceptable.

**Solution** (non-blocking I/O + poll):
```
while true:
    ask operating system: "which clients are ready to read/write?"
    for each ready client:
        do a small amount of work (read some bytes, write some bytes)
        if not finished, we'll continue next iteration
    repeat
```

Now all clients make progress concurrently. 
The server never waits - if one client isn't ready, serve others.

**Analogy**: Restaurant kitchen with multiple orders cooking. 
Don't stare at one pot waiting for it to boil - check other dishes, prep ingredients, 
plate finished orders. When that pot boils, you'll hear the timer.

`poll()` is your timer system - tells you when each client is ready for attention.


### Challenge 2: Partial Data Arrival

**Problem**: Network sends data in chunks. When you call `read()`, you might get:
- The full request (lucky!)
- Part of the request (common)
- Zero bytes because data hasn't arrived yet (very common with non-blocking)

You must **remember state** between reads:

```
Connection {
    socket
    state: READING_REQUEST_LINE | READING_HEADERS | READING_BODY | WRITING_RESPONSE
    buffer: all bytes received so far
    bytes_expected: from Content-Length header
    bytes_received: counter
}
```

Each call to `read()` appends to buffer. 
When `bytes_received == bytes_expected`, request is complete.

**Analogy**: Receiving a long fax page by page. 
Don't throw away earlier pages while waiting for the rest. 
Stack them up until you have the complete document.



### Challenge 3: Parsing HTTP Messages

HTTP has specific structure (from RFC 1945):

```
GET /index.html HTTP/1.0
Host: example.com
User-Agent: Mozilla/5.0

[optional body]
```

**Must parse this correctly**:
- Extract method (GET), URI (/index.html), version (HTTP/1.0)
- Parse headers (name: value pairs)
- Handle body if present (Content-Length tells you how much)

**Parser is a state machine**:
```
START → METHOD → URI → VERSION → HEADER_NAME → HEADER_VALUE → BODY → DONE
```

Each byte you read advances the state machine. 
Invalid input → ERROR state → return 400 Bad Request.

**Analogy**: Following assembly instructions step-by-step. 
Each instruction (character) tells you what to do next. 
Skip a step or do them out of order = broken product.



## The Event Loop: Heart of the Server

Your entire server runs on one loop:

```cpp
while (server_running) {
    // 1. Ask OS: which sockets are ready?
    int ready_count = poll(all_sockets, timeout);
    
    // 2. For each ready socket:
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
    
    // 3. Accept new connections if listening socket ready
    if (listening_socket_ready) {
        accept_new_client();
    }
}
```

**Key insight**: Never block. 
`poll()` blocks for you - tells you when work is available. 
When it wakes up, do available work quickly, then back to `poll()`.

**Analogy**: Air traffic controller watching radar screen. 
Planes (clients) moving at different speeds. 
Controller gives instructions to each when needed, 
never focuses on one plane so long that others crash.



## HTTP Message Structure (The Grammar)

Every HTTP request follows this pattern:

```
Request = Request-Line + Headers + [Body]

Request-Line = Method + Space + URI + Space + HTTP-Version + CRLF
Method = "GET" | "POST" | "DELETE"
Headers = (Header-Name + ":" + Header-Value + CRLF)*
Body = [bytes of length Content-Length]
```

Example:
```
POST /submit HTTP/1.0\r\n
Content-Type: application/json\r\n
Content-Length: 27\r\n
\r\n
{"name":"John","age":30}
```

**Critical details**:
- `\r\n` (CRLF) ends each line (carriage return + line feed)
- Blank line (`\r\n\r\n`) separates headers from body
- `Content-Length` header tells you body size
(without it, no body or read until connection closes)

Your parser must **strictly follow this grammar**. 
Browsers are forgiving; your server should reject malformed requests (400 Bad Request).


## Response Generation: The Other Side

After parsing request and fetching resource, you construct response:

```
HTTP/1.0 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 43\r\n
\r\n
<html><body>Hello World</body></html>
```

**Status codes** (RFC 1945 Section 6):
- 200 OK: Success
- 404 Not Found: Resource doesn't exist
- 400 Bad Request: Malformed request
- 500 Internal Server Error: Server bug
- 403 Forbidden: Access denied

Your server must choose correct status code and send properly formatted response.


## Configuration: Mapping URLs to Resources

Your config file defines how URLs map to files/scripts:

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

Internally, build a lookup table:

```cpp
struct LocationConfig {
    string path_prefix;      // "/cgi-bin/"
    string root_directory;   // "/var/www/html"
    vector<string> index_files;
    optional<string> cgi_handler;
};

map<string, LocationConfig> locations;
```

**URL resolution algorithm**: Longest prefix match.

Request: `GET /cgi-bin/script.py`
- Matches `"/"`
- Also matches `"/cgi-bin/"`
- Choose `"/cgi-bin/"` (more specific)
- Check if CGI handler configured
- If yes, execute script; if no, serve as static file



## CGI: External Program Execution

CGI (Common Gateway Interface) allows server 
to execute programs that generate responses dynamically.

**How it works**:

1. Client requests `/cgi-bin/form.py`
2. Server recognizes this as CGI (from config)
3. Server forks child process
4. Sets environment variables:
   - `REQUEST_METHOD=POST`
   - `CONTENT_LENGTH=27`
   - `QUERY_STRING=user=john`
5. Executes `/usr/bin/python3 form.py`
6. Writes request body to CGI stdin
7. Reads CGI stdout (which is HTTP headers + body)
8. Sends CGI output to client

**Critical**: CGI process runs in parallel. 
Server uses `poll()` on pipes (file descriptors connecting server to CGI process) to avoid blocking.

**Analogy**: Restaurant kitchen outsourcing dessert to nearby bakery. 
Kitchen takes order, sends it to bakery, serves other dishes while waiting, 
picks up dessert when ready, delivers to customer.


## Key Data Structures

### Connection State
```cpp
struct Connection {
    int socket_fd;
    enum State { READING, PROCESSING, WRITING, CLOSING } state;
    vector<char> read_buffer;
    vector<char> write_buffer;
    size_t bytes_written;
    Request parsed_request;
    Response prepared_response;
};
```

### HTTP Request
```cpp
struct Request {
    string method;           // "GET", "POST", "DELETE"
    string uri;              // "/path/to/resource"
    string http_version;     // "HTTP/1.0"
    map<string, string> headers;
    vector<char> body;
};
```

### HTTP Response
```cpp
struct Response {
    int status_code;         // 200, 404, etc.
    string status_text;      // "OK", "Not Found"
    map<string, string> headers;
    vector<char> body;
};
```


## The Connection State Machine

Each connection moves through states:

```
    [ACCEPT]
        ↓
    [READING REQUEST] ←─┐
        ↓               │
        ├─ incomplete ──┘ (stay in READING, wait for more data)
        │
        └─ complete
            ↓
        [PROCESSING]
            ↓
        [WRITING RESPONSE]  ←─┐
            ↓                 │
            ├─ incomplete ────┘ (stay in WRITING, wait for socket ready)
            │
            └─ complete
                ↓
            [CLOSE]
```

**Critical**: Connection can remain in READING or WRITING for multiple `poll()` cycles. 
Must save state between cycles.


## Implementation Strategy: Build in Layers

Incremental validation:

### Layer 1: Echo Server (1 connection)
- Accept connection
- Read bytes
- Echo back
- Close
**Validates**: Socket API, basic I/O

### Layer 2: HTTP Parser
- Parse request line
- Parse headers
- Print parsed structure
**Validates**: State machine, grammar implementation

### Layer 3: Static File Server
- Parse request
- Map URI to file
- Read file
- Send HTTP response
**Validates**: Resource resolution, response generation

### Layer 4: Multiple Connections
- Track N connections
- Use `poll()` to multiplex
- Each connection independent
**Validates**: Non-blocking I/O, event loop

### Layer 5: Configuration
- Parse config file
- Build route lookup
- Apply per-location settings
**Validates**: Configuration architecture

### Layer 6: CGI
- Fork/exec scripts
- Pipe stdin/stdout
- Parse CGI output
**Validates**: Process management

### Layer 7: Error Handling
- Malformed requests → 400
- Missing files → 404
- Timeouts, disconnects
**Validates**: Robustness, RFC compliance


## Common Pitfalls to Avoid

1. **Blocking on file I/O**: Regular files don't work with `poll()`. 
Only sockets and pipes. Solution: Read files into memory first, 
then write from memory to socket using non-blocking writes.

2. **Buffer overflows**: Always check buffer sizes before writing. 
Never assume request fits in fixed-size buffer.

3. **Memory leaks**: Every `accept()` creates socket. 
Every socket must be closed. Use RAII (`unique_ptr`, destructors) if possible.

4. **Malformed HTTP**: Don't assume clients send valid requests. 
Validate everything. Reject bad input early.

5. **Partial writes**: `write()` may send fewer bytes than requested (non-blocking socket, send buffer full). 
Must track how many bytes sent, retry later.

6. **File descriptor limits**: OS has limit (typically 1024). Check with `ulimit -n`. 
Your server must handle this gracefully.


## Essential Invariants (Always True)

1. Every connection has exactly one state (no undefined states)
2. No blocking operations in event loop
3. Buffers never overflow (bounds checking)
4. File descriptors always closed (no leaks)
5. HTTP responses valid per RFC (correct format)
6. One `poll()` controls all I/O


## Testing Strategy

Before moving to next layer, test current layer thoroughly:

### Manual Testing
```bash
# Test with telnet
telnet localhost 8080
GET / HTTP/1.0

# Test with curl
curl -v http://localhost:8080/
curl -X POST -d "data" http://localhost:8080/submit
```

### Automated Testing
```bash
# Stress test
ab -n 1000 -c 10 http://localhost:8080/

# Compare against NGINX
# Your server should produce same responses for same requests
```

### Edge Cases
- Request with no headers
- Request with very large body
- Client disconnects mid-request
- Multiple requests on same connection (if supporting persistent connections)
- Malformed requests (missing CRLF, invalid method, etc.)


## Understanding Non-Blocking I/O: The Core Insight

**Why non-blocking is necessary** (not just "better performance"):

Suppose 100 clients, blocking I/O:

```
Time to serve all clients = Sum of each client's service time
```

If one client takes 60 seconds (slow network), others wait 60 seconds. 
Server throughput = 1 client/minute at best.

With non-blocking I/O + `poll()`:

```
Time to serve all clients = Max of any client's service time
```

All clients progress in parallel. If one is slow, others aren't affected. 
Server throughput = 100 clients/minute (if each takes 1 second on average).

This isn't optimization - it's **fundamental architectural necessity** for any server handling multiple clients.