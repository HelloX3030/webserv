## 0. Program start

* `main(int ac, char **av)`
* Validate arguments
* If config path missing → error & exit

No syscalls yet beyond `write` for errors.

---

## 1. Parse configuration (offline phase)

**Goal:** Build an *in-memory model* of the server(s)

What you do:

* `open` config file
* `read` whole file
* Lex / parse directives
* Build structures:

  * `Server { host, port, server_name, routes… }`
  * `Location { root, index, methods, cgi, autoindex… }`

Relevant syscalls:

* `open`, `read`, `close`
* Possibly `stat` (validate paths)

⚠️ **No networking yet**
This phase must complete before any socket is created.

---

## 2. Resolve addresses (DNS / protocol setup)

**Goal:** Convert config (`host:port`) into usable socket parameters.

For each configured server:

* Call `getaddrinfo(host, port, hints)`
* Store:

  * `sockaddr`
  * `socklen`
  * protocol family (IPv4/IPv6)

Relevant syscalls:

* `getaddrinfo`
* `freeaddrinfo`

---

## 3. Create listening sockets

**Goal:** One listening socket per `(host, port)`

For each resolved address:

1. `socket()`
2. `setsockopt()`

   * `SO_REUSEADDR` (mandatory)
3. `fcntl()`

   * set **non-blocking**
4. `bind()`
5. `listen()`

Data structures:

* `std::map<int, Server*> listen_fds`
* Each fd maps to a `Server` config

Relevant syscalls:

* `socket`
* `setsockopt`
* `fcntl`
* `bind`
* `listen`

At this point:

> The server is **passively listening**, not accepting yet.

---

## 4. Initialize event loop (poll / epoll / kqueue)

**Goal:** Centralized I/O multiplexing

Depending on platform:

* Linux → `epoll`
* macOS/BSD → `kqueue`
* Portable → `poll` or `select`

Typical setup:

* Create event queue (`epoll_create` / `kqueue`)
* Register:

  * all listening sockets for **READ events**

Data:

* `fd → Connection` map
* `Connection` state machine

Relevant syscalls:

* `epoll_create`, `epoll_ctl`
* or `kqueue`, `kevent`
* or `poll`

---

## 5. Main loop (infinite)

```text
while (running):
    wait for events
    for each ready fd:
        dispatch by fd type
```

Everything from now on is **event-driven**.

---

## 6. Accept new clients

Triggered when:

* listening socket is readable

Steps:

1. `accept()`
2. `fcntl()` → non-blocking
3. Create `Connection` object:

   * client fd
   * input buffer
   * output buffer
   * parsing state
4. Register client fd in poller for **READ**

Relevant syscalls:

* `accept`
* `fcntl`

---

## 7. Read from client socket

Triggered when:

* client fd is readable

Steps:

1. `recv()` (or `read()`)
2. Append to request buffer
3. If `recv == 0` → client closed → cleanup
4. Check:

   * headers complete?
   * content-length satisfied?
   * chunked done?

Relevant syscalls:

* `recv`
* `read`
* `close`

---

## 8. Parse HTTP request

**Pure CPU logic (no syscalls)**

Stages:

1. Request line
   `METHOD SP PATH SP HTTP/VERSION`
2. Headers
3. Body (if any)

Validate:

* method allowed
* HTTP version supported
* headers syntax
* body length

Result:

* Valid `HttpRequest`
* Or error status (`400`, `405`, `411`, etc.)

---

## 9. Match request to config

**Routing phase**

Using:

* `Host` header
* socket local port (`getsockname`)
* URI path

Steps:

1. Select `Server`
2. Select best `Location`
3. Resolve:

   * root
   * index
   * CGI?
   * autoindex?
   * allowed methods

Relevant syscalls:

* `getsockname`
* `stat`
* `access`

---

## 10. Generate response

### A) Static file

1. Build filesystem path
2. `stat()`
3. `open()`
4. `read()` file
5. Close file
6. Build HTTP response

Syscalls:

* `stat`, `open`, `read`, `close`

---

### B) Directory

* If autoindex:

  * `opendir`
  * `readdir`
  * `closedir`
* Else:

  * try index file

---

### C) CGI

1. `pipe()` ×2
2. `fork()`
3. Child:

   * `dup2()` stdin/stdout
   * `execve()` CGI binary
4. Parent:

   * read CGI output
   * `waitpid()`

Syscalls:

* `pipe`, `fork`, `dup2`, `execve`, `waitpid`

---

## 11. Write response to client

Triggered when:

* socket writable

Steps:

1. `send()` or `write()`
2. Handle partial writes
3. When done:

   * close or
   * keep-alive → reset state

Relevant syscalls:

* `send`
* `write`
* `close`

---

## 12. Connection teardown

On:

* error
* timeout
* client close
* response complete (non keep-alive)

Steps:

* unregister fd
* `close()`
* delete `Connection`

---

## Mental model (one sentence)

> **Parse config → create listening sockets → event loop → accept → read → parse HTTP → route → generate response → write → close**
