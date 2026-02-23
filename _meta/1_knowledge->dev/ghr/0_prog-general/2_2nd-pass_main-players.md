# webserv — the main players


## the setup, from the outside

3 things involved when the server is running:

- **your server program** — a process, sitting and waiting
- **a browser** — sends requests, displays responses
- **the filesystem** — where the actual files live on disk

The server program mediates between the other 2.
Browser never touches the filesystem directly.


## inside the server program: 4 players

---

### 1. Config

**What:** a data structure loaded once at startup
from the config file you wrote.

**Contains:**
- which port to listen on
- where files live on disk
- which URLs map to which folders
- rules: allowed methods, max upload size, etc.

**Telos:** answer the question —
*"given this request, what should I do with it?"*

Config is read-only after startup.
Everything else consults it.

---

### 2. Server

**What:** 1 object per `server { }` block in your config.
Holds the config for 1 virtual host.

**Knows:**
- its address and port
- its routing rules (which URL prefix → which folder)

**Does:**
- creates the listening socket (1 per address:port)
- given a parsed request: decides what response to generate

**Does not:**
- talk to clients directly
- run any loop
- track who is connected

Server is essentially a *routing table with a socket*.

---

### 3. Connection

**What:** 1 object per connected client, created on
each new incoming connection, destroyed when done.

**Holds:**
- the client's file descriptor (the OS handle for the connection)
- incoming bytes (request accumulating as they arrive)
- outgoing bytes (response waiting to be sent)
- current state: READING → PARSING → PROCESSING → WRITING

**Telos:** shepherd 1 client through 1 request-response cycle.

Connection is the *unit of work*.

---

### 4. event loop

**What:** heartbeat of whole program.
1 loop, runs forever.

**Does, every iteration:**
- asks the OS: "which of my file descriptors
  are ready for I/O?"
- for each ready fd: do the next step
  - new connection arriving? → create a Connection
  - client sent data? → read it into that Connection's buffer
  - Connection has a full request? → parse, then generate response
  - response ready to send? → write bytes to client socket
  - done? → close, destroy Connection

**Why 1 loop?**
Because you cannot block waiting for 1 client while ignoring all others.
1 `poll()` call watches *all* fds simultaneously.

Event Loop is the *coordinator*.

---


## how they relate

```
Config ─────────────────────────────────┐
  (loaded once, consulted by everyone)  │
                                        ▼
                              Server (routing rules)
                                   │
                                   │ creates
                                   ▼
                            listening socket
                                   │
                    new connection arrives
                                   │
                                   ▼
Event Loop ──────────► creates Connection
     │                       │
     │   polls all fds       │ per client:
     │                       │  read → parse → process → write
     └───────────────────────┘
```


## 1 thing that is not a player: the filesystem

The filesystem is not part of the program.
It's just there. When a request comes in for a file,
program calls `open()` / `read()` — OS syscalls —
and the OS fetches bytes from disk.

Your server is a messenger between network and disk.