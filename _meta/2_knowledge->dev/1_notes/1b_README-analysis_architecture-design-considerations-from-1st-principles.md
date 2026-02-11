# Webserver Architecture: First Principles Analysis

## 0. Ontological Foundation

### What IS a webserver?

Essence: A **stateful byte-stream transformer** operating over network channels.

Formal characterisation:
```
Server: (ByteStream, Config) → ByteStream
where ByteStream ⊆ HTTP-Protocol
      Config = RoutingRules × ResourceLocations × Policies
```

Decomposition into necessary components:

. **Input channels** — listening sockets accepting connections.
. **State space** — connection lifecycles, request accumulation, response generation.
. **Transformation logic** — routing requests through configuration to responses.
. **Output channels** — client sockets transmitting responses.
. **Event coordination** — I/O multiplexing across concurrent connections.

### Telos (purpose)

The webserver exists to:

1. **Mediate** between HTTP protocol (network abstraction) and filesystem/process resources (OS abstractions).
2. **Enforce** access policies defined in configuration.
3. **Maintain** concurrent connection state without blocking.

### Fundamental constraints

Imposed by reality, not convention:

. **Concurrency requirement** — must serve N clients simultaneously without N threads (C10K problem).
. **Protocol compliance** — must speak HTTP/1.1 correctly or fail gracefully.
. **Resource finitude** — file descriptors, memory, CPU are bounded.
. **Ordering preservation** — bytes within TCP stream must maintain sequence.

---

## 1. Logical Necessity Analysis

### Phase ordering: what MUST precede what?

**Necessarily sequential:**

1. **Configuration parsing → Socket creation**
   - Cannot bind socket without knowing address:port.
   - Logical dependency: `bind(fd, addr)` requires `addr` from config.

2. **Socket creation → Event loop registration**
   - Cannot monitor non-existent file descriptor.
   - Logical dependency: `poll(fds)` requires `fds` to exist.

3. **Accept → Client I/O**
   - Cannot read from connection that doesn't exist.
   - Logical dependency: `recv(client_fd)` requires `client_fd` from `accept()`.

4. **Request parsing → Routing**
   - Cannot match route without knowing requested URI.
   - Logical dependency: `route(request)` requires parsed `request.uri`.

5. **Response generation → Transmission**
   - Cannot send non-existent data.
   - Logical dependency: `send(response_bytes)` requires generated `response_bytes`.

**Unnecessarily sequential (artificial boundaries in original document):**

. "Resolve addresses" as separate phase — `getaddrinfo()` is part of socket setup, not independent phase.
. "Initialize event loop" as late phase — event queue can be created early, sockets registered incrementally.

### Core invariants

These must hold at all times:

**Invariant 1: Fd uniqueness**
```
∀ fd₁, fd₂ ∈ active_connections: fd₁ = fd₂ ⇒ connection₁ = connection₂
```
Each file descriptor maps to at most one connection object. Violation causes use-after-close bugs.

**Invariant 2: Event registration completeness**
```
∀ fd ∈ event_queue ⟺ fd ∈ (listen_sockets ∪ active_connections)
```
Every monitored fd has associated metadata. Every connection fd is monitored.

**Invariant 3: State machine validity**
```
∀ connection: state ∈ {READING, PARSING, PROCESSING, WRITING, CLOSING}
                     ∧ transitions form DAG (no cycles except keep-alive reset)
```
State transitions must be acyclic (except explicit connection reuse).

**Invariant 4: Buffer ownership**
```
∀ buffer ∈ {read_buffer, write_buffer}: owned_by ∈ {connection, destroyed}
```
Buffers never outlive their connection. No dangling references.

**Invariant 5: CGI child accounting**
```
∀ spawned_pid: eventually reaped via waitpid()
```
No zombie processes. All children must be reaped.

---

## 2. State Space Analysis

### Connection lifecycle state machine

```
                    ┌──────────────┐
                    │   ACCEPTING  │ (listening socket readable)
                    └──────┬───────┘
                           │ accept()
                           ↓
                    ┌──────────────┐
                    │   READING    │ (accumulating request bytes)
                    └──────┬───────┘
                           │ headers + body complete
                           ↓
                    ┌──────────────┐
                    │   PARSING    │ (request bytes → HttpRequest)
                    └──────┬───────┘
                           │ parse success
                    ┌──────┴───────┐
                    │  PROCESSING  │ (routing + response generation)
                    └──────┬───────┘
                           │ response ready
                    ┌──────┴───────┐
                    │   WRITING    │ (transmitting response bytes)
                    └──────┬───────┘
                           │ transmission complete
                    ┌──────┴───────────┐
                    │  CLOSING/RESET   │
                    └──────────────────┘
                           │
                    ┌──────┴──────────┐
                    │  Connection:     │
                    │  close  or  keep │
                    │  -alive          │
                    └──────────────────┘
```

**Critical insight:** This is ONE state machine PER CONNECTION. Listen sockets are separate entities.

### Transitional predicates

What causes state transitions?

. **ACCEPTING → READING**: `accept()` returns valid fd.
. **READING → PARSING**: Request termination detected:
  - For GET: `\r\n\r\n` seen.
  - For POST: `\r\n\r\n` seen AND `Content-Length` bytes received.
  - For chunked: `0\r\n\r\n` seen.
. **PARSING → PROCESSING**: Parse success (valid HTTP).
. **PARSING → ERROR**: Parse failure (malformed HTTP).
. **PROCESSING → WRITING**: Response generated (headers + body).
. **WRITING → CLOSING**: All bytes transmitted.
. **WRITING → RESET**: All bytes transmitted AND `Connection: keep-alive`.

### Error states (missing from original document)

Every state must have error transitions:

. **READING errors**: `recv() == 0` (client closed), `recv() < 0 && errno != EAGAIN`, timeout.
. **PARSING errors**: Invalid method, malformed headers, HTTP version not supported.
. **PROCESSING errors**: File not found, permission denied, disk full, fork failure.
. **WRITING errors**: `send() < 0 && errno != EAGAIN`, client disconnect mid-response.

All errors → generate appropriate response (if possible) or close connection.

---

## 3. Architectural Degrees of Freedom

### Event loop mechanism

**Options:**

1. **poll()** — POSIX standard, array of `struct pollfd`.
2. **select()** — older, fd_set bitmasks, 1024 fd limit.
3. **epoll** — Linux-specific, O(1) event notification.
4. **kqueue** — BSD/macOS, similar to epoll.

**What varies:**
. API surface area.
. Performance characteristics (O(n) vs O(1) for large N).
. Platform availability.

**What's invariant:**
. All implement "wait until fd(s) ready for I/O".
. All require registration/modification/deletion of interest.
. All provide similar event dispatch pattern.

**First-principles choice:**

Use `poll()` initially because:

1. **Portability** — POSIX-compliant, works everywhere.
2. **Simplicity** — mental model is trivial: array of fds + events.
3. **Adequate** — webserv likely serves < 1000 concurrent connections; O(n) scan acceptable.
4. **Evolvability** — can swap to epoll/kqueue later if profiling shows bottleneck.

Premature optimization to epoll violates simplicity principle.

### Buffer management

**Options:**

**A) Single flat buffer per connection:**
```cpp
struct Connection {
    std::string read_buffer;   // grows as bytes arrive
    std::string write_buffer;  // shrinks as bytes sent
};
```

Pros:
. Simple.
. Standard library handles reallocation.

Cons:
. Potential reallocation on growth.
. Cannot handle arbitrarily large requests (must enforce limit).

**B) Scatter-gather (vector of chunks):**
```cpp
struct Connection {
    std::vector<std::vector<char>> read_chunks;
};
```

Pros:
. No reallocation.
. Can handle large bodies efficiently.

Cons:
. Parsing complexity (must handle chunk boundaries).
. More complex memory management.

**C) Circular buffer:**
```cpp
struct Connection {
    char buffer[BUFFER_SIZE];
    size_t read_pos;
    size_t write_pos;
};
```

Pros:
. Fixed memory footprint.
. Cache-friendly.

Cons:
. Must handle wraparound in parsing.
. Limited by BUFFER_SIZE.

**First-principles choice:**

Use **A (flat buffer)** because:

1. Webserv spec likely enforces max request size (e.g. 1MB).
2. Simplicity dominates for this use case.
3. Modern allocators make reallocation cheap for small-medium sizes.
4. Can evolve to B if profiling reveals necessity.

### CGI execution model

Original document shows:
```
pipe() → fork() → dup2() → execve() → waitpid()
```

**Unstated critical question:** Synchronous or asynchronous CGI?

**Synchronous CGI:**
. Parent blocks on `waitpid()` until child exits.
. Simple but violates non-blocking requirement.
. Unacceptable: one slow CGI blocks entire server.

**Asynchronous CGI (necessary approach):**
. Parent continues event loop while child runs.
. Parent reads CGI output via non-blocking pipe.
. Parent calls `waitpid(pid, &status, WNOHANG)` periodically.

**Consequence:** CGI introduces **sub-state machine** within PROCESSING state:

```
PROCESSING (CGI variant):
├─ CGI_FORKED    (child spawned, pipes open)
├─ CGI_READING   (accumulating CGI stdout)
└─ CGI_COMPLETE  (child reaped)
    └→ continue to WRITING
```

**Implementation necessity:**

Must track:
. `pid_t cgi_pid` per connection.
. Pipe fds for stdin/stdout.
. Register pipe fds in event loop.

**Reaping mechanism:**

Cannot call blocking `waitpid()`. Two approaches:

1. **WNOHANG polling:**
   ```cpp
   for (auto& [pid, conn] : cgi_processes) {
       if (waitpid(pid, &status, WNOHANG) > 0) {
           // child exited
       }
   }
   ```
   Simple but wasteful.

2. **SIGCHLD + signalfd/self-pipe:**
   ```cpp
   signal(SIGCHLD, handler);  // handler writes to pipe
   // poll() wakes when SIGCHLD arrives
   // then waitpid() known to succeed immediately
   ```
   Efficient but more complex.

For webserv: approach 1 adequate. Can optimize later.

---

## 4. Critical Gaps in Original Document

### 1. Timeout handling

Original document: no mention.

**Necessity:**

. Client connects but never sends request → consume fd forever.
. Client sends headers but stalls on body → tie up resources.
. CGI runs indefinitely → leak processes.

**Solution:**

Maintain per-connection timestamp:
```cpp
struct Connection {
    time_t last_activity;  // updated on every I/O
};

// In event loop:
time_t now = time(NULL);
for (auto& [fd, conn] : connections) {
    if (now - conn.last_activity > TIMEOUT) {
        // generate 408 Request Timeout or close
    }
}
```

Typical timeout: 30-60 seconds.

### 2. Resource limits

Original document: assumes infinite resources.

**Reality:**

. File descriptors are limited (ulimit -n, typically 1024).
. Memory is finite.
. CGI processes could fork-bomb.

**Necessary limits:**

```cpp
const size_t MAX_CONNECTIONS = 1000;
const size_t MAX_REQUEST_SIZE = 1024 * 1024;  // 1MB
const size_t MAX_CGI_PROCESSES = 10;
```

When limit reached: return `503 Service Unavailable`.

### 3. HTTP pipelining

Client sends multiple requests on one connection:
```
GET /a HTTP/1.1\r\nHost: foo\r\n\r\n
GET /b HTTP/1.1\r\nHost: foo\r\n\r\n
```

**Options:**

1. **Forbid** — respond with `Connection: close` after first request.
2. **Support** — maintain request queue per connection.

**First-principles analysis:**

Pipelining complicates state machine:
. Must parse multiple requests from read_buffer.
. Must queue responses (HOL blocking if out-of-order).
. Edge cases: partial second request in buffer.

**Recommendation:** Forbid initially. Add later if needed.
Implementation: always send `Connection: close` in response headers.

### 4. Chunked transfer encoding

Original document mentions "chunked done?" but no detail.

**Necessity:** HTTP/1.1 allows:
```
POST /upload HTTP/1.1
Transfer-Encoding: chunked

5\r\n
hello\r\n
0\r\n
\r\n
```

**Parser requirements:**

```
State machine for chunked body:
├─ READ_CHUNK_SIZE   (hex digits until \r\n)
├─ READ_CHUNK_DATA   (n bytes)
├─ READ_CHUNK_CRLF   (\r\n after chunk data)
└─ DONE              (saw 0\r\n\r\n)
```

Cannot simply `read(fd, buf, Content-Length)` as with normal bodies.

**Implementation necessity:**

```cpp
struct ChunkedParser {
    enum State { SIZE, DATA, CRLF, TRAILER, DONE };
    State state;
    size_t chunk_size;
    size_t bytes_read;
    std::string body;  // accumulated chunks
};
```

Non-trivial addition. Consider deferring if webserv spec doesn't require POST.

---

## 5. Simplified Architectural Model

### Essence

```
Server := ListenSockets ∪ Connections
          where ListenSockets: stateless accept machines
                Connections: stateful request-response machines
          
          + EventLoop: coordinator
          + Config: routing table
```

### Core loop (pseudo-code)

```
pollfds = [listen_fds + connection_fds]

loop forever:
    ready = poll(pollfds, timeout=1000ms)
    
    for each fd in ready:
        if fd in listen_fds:
            client_fd = accept(fd)
            connections[client_fd] = new Connection(READING)
            pollfds.add(client_fd, POLLIN)
        
        else if fd in connections:
            conn = connections[fd]
            
            match conn.state:
                READING:
                    bytes = recv(fd)
                    conn.read_buffer += bytes
                    if request_complete(conn.read_buffer):
                        conn.state = PARSING
                
                PARSING:
                    conn.request = parse_http(conn.read_buffer)
                    if valid:
                        conn.state = PROCESSING
                    else:
                        conn.response = generate_error(400)
                        conn.state = WRITING
                
                PROCESSING:
                    conn.response = route_and_generate(conn.request, config)
                    conn.state = WRITING
                    pollfds.modify(fd, POLLOUT)  // switch to write interest
                
                WRITING:
                    sent = send(fd, conn.write_buffer)
                    conn.write_buffer.erase(0, sent)
                    if conn.write_buffer.empty():
                        if keep_alive:
                            reset_connection(conn)
                        else:
                            close(fd)
                            connections.erase(fd)
                            pollfds.remove(fd)
    
    // Timeout check
    now = time()
    for each conn in connections:
        if now - conn.last_activity > TIMEOUT:
            close_connection(conn)
```

### Data structures

```cpp
// Config loaded at startup
struct Server {
    std::string host;
    uint16_t port;
    std::vector<std::string> server_names;
    std::map<std::string, Location> locations;
};

struct Location {
    std::string root;
    std::vector<std::string> index_files;
    std::set<std::string> allowed_methods;
    bool autoindex;
    std::optional<std::string> cgi_path;
};

// Runtime state
enum ConnectionState {
    READING,
    PARSING,
    PROCESSING,
    WRITING
};

struct Connection {
    int fd;
    ConnectionState state;
    time_t last_activity;
    
    std::string read_buffer;
    std::string write_buffer;
    size_t write_offset;
    
    HttpRequest request;    // parsed
    HttpResponse response;  // generated
    
    // CGI state (if applicable)
    std::optional<pid_t> cgi_pid;
    std::optional<int> cgi_stdout_fd;
};

// Event loop
std::vector<pollfd> pollfds;
std::map<int, Server*> listen_fds;
std::map<int, std::unique_ptr<Connection>> connections;
```

---

## 6. Implementation Phases (Upstream to Downstream)

### Phase 0: Conceptual model validation

Before any code:

1. Draw state machine diagrams.
2. List all invariants.
3. Identify error conditions.
4. Design data structures.

### Phase 1: Minimal viable server

Constraints:
. Single listen socket (hardcoded localhost:8080).
. No configuration file.
. GET requests only.
. Static files only (no CGI).
. No keep-alive (always close after response).
. Use `poll()`.

**Why:** Establishes core event loop and state machine.

Test:
```bash
echo -e "GET /index.html HTTP/1.1\r\n\r\n" | nc localhost 8080
```

### Phase 2: Configuration and routing

Add:
. Config file parsing.
. Multiple servers (virtual hosts).
. Location matching.
. Index file resolution.
. Directory listing (autoindex).

**Why:** Introduces routing logic without concurrency complexity.

### Phase 3: POST and body handling

Add:
. Content-Length body reading.
. Method validation.
. 411/413 error handling.

Defer: Chunked transfer encoding (if not required).

**Why:** Essential for practical HTTP but no new concurrency issues.

### Phase 4: CGI execution

Add:
. Fork/exec mechanism.
. Pipe handling.
. Non-blocking CGI output reading.
. Process reaping.

**Why:** Introduces sub-state machine, tests async handling.

### Phase 5: Robustness

Add:
. Timeout handling.
. Resource limits.
. Error recovery.
. Graceful shutdown (SIGINT/SIGTERM).

**Why:** Production-readiness.

### Phase 6: Optimization (if needed)

Consider:
. epoll/kqueue instead of poll.
. Scatter-gather buffers.
. sendfile() for static files.

**Why:** Only if profiling reveals bottleneck.

---

## 7. Testing Strategy

### Unit tests (pure functions)

. HTTP request parser: valid and malformed inputs.
. HTTP response generator: status codes, headers, body.
. Config parser: valid and invalid syntax.
. Route matcher: URI → Location resolution.

### Integration tests (black box)

```bash
# Test 1: Basic GET
curl -v http://localhost:8080/

# Test 2: Static file
curl http://localhost:8080/test.html

# Test 3: Directory listing
curl http://localhost:8080/files/

# Test 4: POST with body
curl -X POST -d "key=value" http://localhost:8080/form

# Test 5: CGI
curl http://localhost:8080/cgi-bin/test.py

# Test 6: Concurrent clients
seq 100 | xargs -P 10 -I {} curl http://localhost:8080/

# Test 7: Timeout
(echo -ne "GET / HTTP/1.1\r\n"; sleep 70) | nc localhost 8080

# Test 8: Malformed request
echo -ne "INVALID\r\n\r\n" | nc localhost 8080
```

### Stress tests

```bash
# Apache Bench
ab -n 10000 -c 100 http://localhost:8080/

# Siege
siege -c 50 -t 1M http://localhost:8080/
```

### Error injection

. Disk full (fill /tmp).
. Fd exhaustion (ulimit -n 100).
. Kill CGI child mid-execution.
. Close client socket mid-response.

---

## 8. Invariant Validation

### Runtime assertions

```cpp
void validate_invariants() {
    // Inv 1: Fd uniqueness
    std::set<int> seen_fds;
    for (const auto& [fd, _] : connections) {
        assert(seen_fds.insert(fd).second);
    }
    
    // Inv 2: Event registration
    for (const auto& pfd : pollfds) {
        assert(connections.count(pfd.fd) || listen_fds.count(pfd.fd));
    }
    
    // Inv 3: Valid fds
    for (const auto& [fd, _] : connections) {
        assert(fcntl(fd, F_GETFD) != -1);  // fd is open
    }
    
    // Inv 4: State validity
    for (const auto& [_, conn] : connections) {
        assert(conn->state >= READING && conn->state <= WRITING);
    }
}
```

Call after every event loop iteration in debug builds.

### Logging state transitions

```cpp
void log_transition(Connection* conn, ConnectionState from, ConnectionState to) {
    static const char* names[] = {"READING", "PARSING", "PROCESSING", "WRITING"};
    fprintf(stderr, "[%d] %s → %s\n", conn->fd, names[from], names[to]);
}
```

Produces execution trace:
```
[4] READING → PARSING
[4] PARSING → PROCESSING
[4] PROCESSING → WRITING
[4] WRITING → CLOSED
```

Essential for debugging state machine bugs.

---

## 9. Ontological Summary

**What a webserver IS:**

A **finite-state machine coordinator** managing concurrent byte-stream transformations according to protocol rules and configuration policy.

**What it MUST do:**

. Accept TCP connections without blocking.
. Parse HTTP according to RFC 2616/7230.
. Route requests through configuration space.
. Access filesystem or spawn CGI processes.
. Generate compliant HTTP responses.
. Maintain invariants across all states.
. Release resources deterministically.

**What it MUST NOT do:**

. Block on I/O (violates concurrency).
. Assume infinite resources (violates finiteness).
. Trust client input (violates security).
. Leak file descriptors or memory (violates determinism).

**How it achieves purpose:**

Through rigorous state machine design where:
. States are explicit and finite.
. Transitions are triggered by events (I/O readiness, timeouts).
. Invariants are maintained across all transitions.
. Errors are handled systematically at every level.

---

## 10. Closing Synthesis

The webserver architecture reduces to:

**1. Event-driven state machines** (one per connection) coordinated by **2. I/O multiplexing** (poll/epoll/kqueue) guided by **3. Configuration-based routing** enforcing **4. Protocol compliance** (HTTP) while maintaining **5. Resource invariants** (fd management, memory, processes).

Every design decision must trace to necessity:
. Why this state? → Required by HTTP protocol state.
. Why this data structure? → Maintains invariant X.
. Why this syscall? → Only way to achieve Y without blocking.