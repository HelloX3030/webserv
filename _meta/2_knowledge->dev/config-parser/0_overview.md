# Config Parser — Overview

## Essence

The config parser is a **pure function**:

```
bytes (config file) → [Server]
```

No I/O. No networking. No side effects. 
It reads text and produces data structures.

---

## Position in the System

The system has 3 phases:

```
1. PARSE     config file → Server[] (configuration data)
2. INIT      Server[] → open sockets, register fds with epoll
3. RUN       event loop — accept, read, route, write
```

The parser owns phase 1 entirely. 
It hands off `std::vector<Server>` to the runtime.

```
config file
    │
    ▼
ConfigParser::parse()
    │
    ▼
std::vector<Server>      ← parser's sole output
    │
    ▼
WebServ::run()           ← runtime takes over
```

---

## Telos

Produce a validated, complete, runtime-ready representation of the operator's intent.

"Runtime-ready" means: whatever `WebServ::run()` needs.

---

## What It Is Not

- Not part of the event loop
- Not responsible for socket creation
- Not responsible for request handling
- Not a general-purpose config system (no variables, no inheritance, no includes)

---

## Input / Output Contract

**Input:** path to a config file (NGINX-style syntax)

**Output:** `std::vector<Server>` — each `Server` holds:
- host, port
- server_names
- client_max_body_size
- error_pages map
- locations map → each `Location` holds root, index files, 
allowed methods, autoindex, CGI config

**On any error:** throw with a precise, line-numbered message. 
The program must not start with a malformed config.