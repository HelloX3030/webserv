# Michael Kerrisk: The Linux Programming Interface

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


Significance for WebServ: 
    More modern than Stevens. Covers Linux-specific APIs.