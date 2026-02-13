# Webserv: Specification-Driven Architecture

ctx:

Attempted first-principles reasoning (with help of AI)
in combination with foundations-focused study is good, but too slow.

Need to be pragmatic and make progress with program dev.
Good enough is good enough...


## 0. Starting Point: What Must We Actually Build?

From the subject specification, extract requirements:

**Must handle:**
- HTTP/1.1 GET, POST, DELETE methods
- Static file serving
- CGI execution
- File uploads
- Multiple server configurations (virtual hosts)
- Non-blocking I/O (no hanging on client operations)
- Configuration file parsing

**Must not:**
- Block the entire server on slow client
- Leak resources (fds, memory, processes)
- Crash on malformed input

**May simplify:**
- HTTP/1.1 subset (no pipelining if not required)
- Single-threaded event loop (no multi-threading complexity)
- Fixed resource limits (max connections, max request size)

This defines the problem space. Now: what's the minimal sufficient solution?

---

## 1. Core Architecture: Three Components

```
┌─────────────────┐
│  Config Parser  │ (startup phase)
└────────┬────────┘
         │ produces
         ↓
┌─────────────────┐
│  Server Config  │ (data)
└────────┬────────┘
         │ used by
         ↓
┌─────────────────┐
│   Event Loop    │ (runtime)
└─────────────────┘
```

**Why this separation?**

Parse once, use many times. Config is read-only after startup.
Event loop has no parsing concerns. Clean boundary.

---



## 2. Event Loop: The Heart


### Minimal model

```
while (running):
    events = poll(all_fds, timeout)

    for each ready_fd:
        if ready_fd is listen_socket:
            handle_accept(ready_fd)
        else:
            handle_client(ready_fd)
```

That's it. Two cases: accepting new connections, or servicing existing ones.


### What's inside handle_client?

This is where state matters.


### Approach A: Implicit state (data presence indicates state)

```cpp
struct Connection {
    int fd;

    // State is implicit:
    std::string request_buffer;   // accumulating
    HttpRequest* parsed_request;  // nullptr until parsed
    HttpResponse* response;       // nullptr until generated

    size_t bytes_sent;  // 0 until writing
};

void handle_client(Connection* conn) {
    if (conn->response) {
        // We have a response → write it
        send_response(conn);
    }
    else if (conn->parsed_request) {
        // We have parsed request → generate response
        conn->response = generate_response(conn->parsed_request);
    }
    else {
        // Still reading/parsing request
        read_and_parse(conn);
    }
}
```

**Pros:**
- Natural control flow
- State is obvious from data
- Less ceremony

**Cons:**
- Harder to log "current state" for debugging
- Must check multiple pointers to determine state
- Edge cases: what if response exists but also want to read more? (keep-alive)


### Approach B: Explicit state enum

```cpp
enum State { READING, PROCESSING, WRITING };

struct Connection {
    int fd;
    State state;

    std::string request_buffer;
    HttpRequest* parsed_request;
    HttpResponse* response;
    size_t bytes_sent;
};

void handle_client(Connection* conn) {
    switch (conn->state) {
        case READING:
            if (read_request(conn)) {
                conn->parsed_request = parse(conn->request_buffer);
                conn->state = PROCESSING;
            }
            break;

        case PROCESSING:
            conn->response = generate_response(conn->parsed_request);
            conn->state = WRITING;
            break;

        case WRITING:
            send_response(conn);
            if (done) conn->state = READING;  // keep-alive
            break;
    }
}
```

**Pros:**
- Explicit transitions visible
- Easy to log/debug ("Connection 5 is in WRITING state")
- Clear separation of concerns per state

**Cons:**
- More code
- Enum + data redundancy (state vs pointer presence)
- Can get verbose if many states


### Approach C: Hybrid (minimal explicit state)

```cpp
struct Connection {
    int fd;
    bool reading_complete;  // single flag

    std::string request_buffer;
    HttpRequest* parsed_request;
    HttpResponse* response;
    size_t bytes_sent;
};

void handle_client(Connection* conn) {
    if (!conn->reading_complete) {
        if (read_request(conn)) {
            conn->reading_complete = true;
            conn->parsed_request = parse(conn->request_buffer);
        }
    }

    if (conn->reading_complete && !conn->response) {
        conn->response = generate_response(conn->parsed_request);
    }

    if (conn->response) {
        send_response(conn);
    }
}
```

**Pros:**
- Single flag instead of enum
- Still mostly implicit
- Slightly clearer than pure implicit

**Cons:**
- Mixes paradigms awkwardly


### Recommendation

Start with **Approach A (implicit)** for initial implementation. Why?

1. Webserv is small enough that implicit state is comprehensible.
2. If debugging becomes hard, refactor to Approach B.
3. Simpler code → faster iteration → more learning.

The state IS there (in the data). Making it explicit doesn't add information.

**Exception:** If you find yourself checking many conditions to determine "what to do next,"
that's the signal to add explicit state.



---



## 3. Connection Lifecycle: Following the Data


### What data exists when?

```
accept() creates fd
    ↓
read() populates request_buffer
    ↓
parse() creates HttpRequest object
    ↓
route() + generate() creates HttpResponse object
    ↓
send() transmits response bytes
    ↓
close() destroys fd
```

The data flow IS the state flow.


### Concrete structure

```cpp
struct Connection {
    int fd;
    time_t last_activity;  // for timeout

    // Request side
    std::string request_buffer;
    HttpRequest* request;  // heap allocated, owned

    // Response side
    HttpResponse* response;  // heap allocated, owned
    size_t response_offset;  // bytes already sent

    // CGI (if applicable)
    pid_t cgi_pid;  // 0 if not CGI
    int cgi_pipe_fd;  // -1 if not CGI
};
```

**Ownership rules:**
- Connection owns request and response (delete in destructor)
- Connection owns CGI child (must reap before destruction)
- EventLoop owns Connection (via unique_ptr in map)

Clear ownership → no leaks.


### Lifecycle transitions

```
Connection created:
    fd = accept()
    last_activity = now()
    all pointers = nullptr
    all fds = -1

While reading:
    recv() → request_buffer
    if complete: request = parse(request_buffer)

When request ready:
    response = generate(request, config)
    if CGI: fork and set cgi_pid, cgi_pipe_fd

While writing:
    send() → response data
    response_offset tracks progress

Connection destroyed:
    if (cgi_pid > 0): waitpid()
    if (cgi_pipe_fd != -1): close()
    close(fd)
    delete request
    delete response
```



---



## 4. Event Registration Strategy


### The core problem

poll() takes array of `struct pollfd`.
How do we map events back to connections?


### Option 1: Parallel arrays

```cpp
std::vector<pollfd> fds;
std::vector<Connection*> connections;
```

**Pro:** Simple.
**Con:** Must keep in sync. Error-prone.


### Option 2: Map lookup

```cpp
std::vector<pollfd> fds;
std::map<int, Connection*> fd_to_conn;
```

**Pro:** Clear association.
**Con:** Two structures to maintain.


### Option 3: Single map + rebuild fds

```cpp
std::map<int, std::unique_ptr<Connection>> connections;

// Each loop iteration:
std::vector<pollfd> fds;
for (auto& [fd, conn] : connections) {
    fds.push_back({fd, POLLIN | POLLOUT, 0});
}
fds.push_back(listen_socket);  // always readable

int ready = poll(fds.data(), fds.size(), timeout);
```

**Pro:** Single source of truth (connections map).
**Con:** Rebuilding fds array each iteration.

**Performance:** For < 1000 connections, rebuilding is negligible (few microseconds).

**Recommendation:** Option 3. Simplicity > premature optimization.



---



## 5. Request Completion Detection


### The critical question

How do you know when to stop reading and start parsing?


### For requests without body (GET, DELETE)

```
Look for: \r\n\r\n
```

Simple string search in request_buffer.


### For requests with body (POST)

```
1. Read until \r\n\r\n (headers complete)
2. Parse Content-Length header
3. Continue reading until buffer.size() == headers_len + content_length
```


### For chunked encoding

Requires chunk parser state machine. Defer unless spec requires it.


### Implementation

```cpp
bool request_complete(const std::string& buffer, const HttpRequest* req) {
    size_t header_end = buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false;  // headers not done
    }

    if (!req) {
        return false;  // need parsed headers to check body
    }

    if (req->method == "GET" || req->method == "DELETE") {
        return true;  // no body expected
    }

    // Check Content-Length
    size_t expected_body = req->content_length;
    size_t actual_body = buffer.size() - (header_end + 4);
    return actual_body >= expected_body;
}
```



---



## 6. Response Generation: The Routing Phase


### From request to response

```
HttpRequest → Config → Location → Resource → HttpResponse
```


### Steps

1. **Select server**: Match `Host` header to configured server_names.
2. **Select location**: Match request URI to location blocks.
3. **Resolve resource**: Apply root, check if file/directory/CGI.
4. **Generate response**: Build headers + body.


### Example

```cpp
HttpResponse* generate_response(HttpRequest* req, Config* config) {
    Server* srv = select_server(req->host, config);
    Location* loc = select_location(req->uri, srv);

    std::string path = resolve_path(req->uri, loc);

    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return error_response(404);
    }

    if (S_ISDIR(st.st_mode)) {
        if (loc->autoindex) {
            return directory_listing(path);
        } else {
            path = try_index_files(path, loc);
            if (path.empty()) return error_response(403);
        }
    }

    if (is_cgi(path, loc)) {
        return execute_cgi(req, path);
    }

    return serve_file(path);
}
```

Straightforward control flow. No fancy patterns needed.

---

## 7. CGI: The Complex Case

### Why CGI is special

It's the only case where response generation is asynchronous.

### Synchronous CGI (simple but wrong)

```cpp
pid = fork();
if (pid == 0) {
    execve(cgi_path);
}
waitpid(pid);  // BLOCKS entire server
```

Violates non-blocking requirement.

### Asynchronous CGI (correct)

```cpp
int stdout_pipe[2];
pipe(stdout_pipe);

pid = fork();
if (pid == 0) {
    dup2(stdout_pipe[1], STDOUT_FILENO);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    execve(cgi_path);
}

close(stdout_pipe[1]);  // parent doesn't write
fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);

// Store in connection:
conn->cgi_pid = pid;
conn->cgi_pipe_fd = stdout_pipe[0];
```

### Reading CGI output

In event loop:
```cpp
if (conn->cgi_pipe_fd != -1) {
    char buf[4096];
    ssize_t n = read(conn->cgi_pipe_fd, buf, sizeof(buf));

    if (n > 0) {
        conn->response->append_body(buf, n);
    }
    else if (n == 0) {
        // EOF: CGI done writing
        close(conn->cgi_pipe_fd);
        conn->cgi_pipe_fd = -1;
        // Now response is complete
    }
}
```

### Reaping the process

```cpp
if (conn->cgi_pid > 0 && conn->cgi_pipe_fd == -1) {
    int status;
    if (waitpid(conn->cgi_pid, &status, WNOHANG) > 0) {
        conn->cgi_pid = 0;
        // Check status for errors if needed
    }
}
```

Call this for all connections each loop iteration.


### Key insight

CGI turns Connection into a mini state machine:
- Fork phase
- Reading phase (pipe open)
- Reaping phase (pipe closed, process alive)
- Done (process reaped)

But we don't need explicit states. The data tells us:
- `cgi_pid == 0` → no CGI or done
- `cgi_pid > 0 && cgi_pipe_fd != -1` → reading
- `cgi_pid > 0 && cgi_pipe_fd == -1` → waiting to reap

---



## 8. Simplification Decisions


### What we're NOT doing (unless spec requires)

**HTTP pipelining:**
Always send `Connection: close`. One request per connection lifecycle.

**Chunked transfer encoding:**
Require `Content-Length` for POST. Return 411 if missing.

**Range requests:**
Always send full file. No partial content support.

**Persistent connections (keep-alive):**
Close after each response. Simplifies state management.

**Virtual hosting on IP:**
Match only on `Host` header. One socket can serve multiple servers.


### What we ARE doing

**Non-blocking I/O:**
Essential. Cannot compromise.

**Multiple servers:**
Required by spec. Each server block in config gets matched.

**CGI:**
Required by spec. Async execution as shown.

**Error handling:**
All edge cases must return appropriate HTTP status.

---

## 9. Data Structures Summary

```cpp
// Configuration (loaded once at startup)
struct Location {
    std::string root;
    std::vector<std::string> index;
    std::set<std::string> allowed_methods;
    bool autoindex;
    std::string cgi_extension;  // e.g., ".py"
};

struct Server {
    std::string host;
    uint16_t port;
    std::vector<std::string> server_names;
    std::map<std::string, Location> locations;  // path → location
};

struct Config {
    std::vector<Server> servers;
};

// Runtime (dynamic)
struct HttpRequest {
    std::string method;
    std::string uri;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    size_t content_length;
};

struct HttpResponse {
    int status_code;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string to_string() const {
        // Format: "HTTP/1.1 200 OK\r\n..."
    }
};

struct Connection {
    int fd;
    time_t last_activity;

    std::string request_buffer;
    HttpRequest* request;

    HttpResponse* response;
    size_t bytes_sent;

    pid_t cgi_pid;
    int cgi_pipe_fd;
};

// Event loop state
std::map<int, std::unique_ptr<Connection>> connections;
std::vector<int> listen_sockets;  // can have multiple
Config config;
```

---

## 10. Error Handling Strategy

### Categories

**Client errors (4xx):**
Return error response, close connection.

Examples:
- 400 Bad Request (malformed HTTP)
- 404 Not Found
- 405 Method Not Allowed
- 413 Payload Too Large

**Server errors (5xx):**
Log error, return 500, close connection.

Examples:
- 500 Internal Server Error (unexpected exception)
- 501 Not Implemented (unsupported feature)
- 503 Service Unavailable (resource exhausted)

### Implementation

```cpp
HttpResponse* error_response(int code) {
    HttpResponse* resp = new HttpResponse;
    resp->status_code = code;
    resp->headers["Content-Type"] = "text/html";
    resp->body = format_error_page(code);
    return resp;
}

void handle_client_error(Connection* conn, int code) {
    if (!conn->response) {
        conn->response = error_response(code);
    }
}
```

### Timeout handling

```cpp
time_t now = time(NULL);
for (auto it = connections.begin(); it != connections.end(); ) {
    if (now - it->second->last_activity > TIMEOUT) {
        close(it->second->fd);
        it = connections.erase(it);
    } else {
        ++it;
    }
}
```

Run this check every N loop iterations (e.g., every 10 polls).

---

## 11. Implementation Sequence

### Phase 1: Echo server

Target: Accept connection, read bytes, echo them back.

**Goal:** Validate event loop mechanics without HTTP complexity.

```bash
# Test:
echo "hello" | nc localhost 8080
# Should see "hello" echoed back
```

### Phase 2: Static GET

Add:
- HTTP request parsing (minimal: just request line)
- File serving
- Response formatting

**Goal:** Serve HTML files for GET requests.

```bash
curl http://localhost:8080/index.html
```

### Phase 3: Configuration

Add:
- Config file parser
- Server selection
- Location matching

**Goal:** Multiple virtual hosts working.


### Phase 4: POST + uploads

Add:
- Content-Length handling
- Body reading
- File upload (save to disk)

**Goal:** HTML form with file upload works.


### Phase 5: CGI

Add:
- Fork/exec mechanism
- Pipe handling
- Process reaping

**Goal:** CGI scripts execute and return output.


### Phase 6: Robustness

Add:
- All error codes
- Timeout handling
- Resource limits
- Graceful shutdown

**Goal:** Server handles malformed input gracefully.



---



## 12. Testing Approach

### Manual testing sequence

```bash
# 1. Server starts
./webserv config.conf
# Should print "Listening on localhost:8080" and not crash

# 2. Basic GET
curl -v http://localhost:8080/
# Should return index.html with 200 OK

# 3. 404
curl -v http://localhost:8080/nonexistent
# Should return 404

# 4. POST
curl -X POST -d "key=value" http://localhost:8080/upload
# Should save file

# 5. CGI
curl http://localhost:8080/cgi-bin/test.py
# Should execute and return output

# 6. Concurrent
seq 10 | xargs -P 10 -I {} curl http://localhost:8080/
# All should succeed

# 7. Timeout
(echo -ne "GET"; sleep 70) | nc localhost 8080
# Should timeout and close

# 8. Malformed
echo "INVALID REQUEST" | nc localhost 8080
# Should return 400 and close
```

### Automated testing

Use provided tester or write simple shell script:
```bash
#!/bin/bash
for test in tests/*.sh; do
    echo "Running $test"
    bash "$test" || exit 1
done
echo "All tests passed"
```



---



## 13. Pragmatic Decisions Log

Document decisions and rationale:

**Decision 1: No explicit state enum (initially)**
- Rationale: Implicit state from data presence is sufficient for webserv scale
- Revisit if: Debugging becomes difficult or state transitions unclear

**Decision 2: Rebuild poll array each iteration**
- Rationale: Simplicity, negligible performance impact for < 1000 connections
- Revisit if: Profiling shows this is bottleneck

**Decision 3: Single-threaded**
- Rationale: Spec doesn't require multi-threading, event loop sufficient
- Revisit if: Cannot meet performance requirements

**Decision 4: No keep-alive**
- Rationale: Simplifies connection lifecycle, one request per connection
- Revisit if: Spec explicitly requires persistent connections

**Decision 5: WNOHANG polling for CGI**
- Rationale: Simple, adequate for low CGI frequency
- Revisit if: Many concurrent CGI processes cause inefficiency

Each decision is:
- Justified by current constraints
- Documented for future revision
- Not permanent


---

## 14. Closing Philosophy

**Simplicity is not laziness.**

Simplicity is:
- Choosing the minimal structure that solves the problem
- Avoiding accidental complexity
- Making the system comprehensible

**Premature optimization is the enemy.**

First: make it work correctly.
Then: make it fast (if needed).

**Explicit vs Implicit is context-dependent.**

In C++:
- Explicit types (int, char*) are necessary
- Explicit state (enums) are optional

Choose explicitness when:
- It clarifies what would otherwise be obscure
- It prevents errors
- It aids debugging

Avoid explicitness when:
- It merely restates what's already clear
- It adds ceremony without insight
- It makes code harder to read

**For webserv:**

The spec defines the problem.
The simplest correct solution is the right one.
Complexity can be added later if measurement shows need.