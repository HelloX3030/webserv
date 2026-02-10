1. RFC 1945

Focus on
Section 4 (HTTP Message), Section 5 (Request), Section 6 (Response). 
These define the grammar to implement.

Qs, e.g.:
What are the valid characters in each field?
What are the size limits?
What happens if client violates the grammar?
Which parts are optional vs required?


2. Stevens Chapter 6 Critical Points

Focus on:

    Why select() and poll() exist (the blocking problem proof)
    The exact semantics of poll() return values
    What POLLIN, POLLOUT, POLLERR, POLLHUP mean
    How to handle EAGAIN/EWOULDBLOCK
    The descriptor set management