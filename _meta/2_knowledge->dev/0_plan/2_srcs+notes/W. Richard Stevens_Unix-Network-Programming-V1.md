# W. Richard Stevens: Unix Network Programming Vol 1 (3rd Edition)

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


Significance for WebServ:

    Explains *why* systems designed this way.
    Proves necessity from first principles. 
    Mathematical rigor meets practical clarity.