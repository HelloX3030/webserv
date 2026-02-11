## Accessible Learning Resources (Bridge to RFCs)
Before diving into Stevens, get intuition:


MDN Web Docs: HTTP - https://developer.mozilla.org/en-US/docs/Web/HTTP

    Clear explanations with examples
    HTTP message structure visualized
    Status codes, headers explained



Julia Evans' Networking Zines - https://wizardzines.com/

    "Networking! ACK!" and "HTTP: Learn your browser's language"
    Comic format, very accessible
    Demystifies concepts brilliantly



HTTP Made Really Easy - https://www.jmarshall.com/easy/http/

    Simple tutorial format
    Shows actual telnet sessions
    Good for hands-on understanding



Computer Networking: A Top-Down Approach (Kurose & Ross)

    Chapter 2: Application Layer (covers HTTP)
    More pedagogical than Stevens
    Good diagrams and examples



Beej's Guide to Network Programming - https://beej.us/guide/bgnet/

    Free online
    C socket programming tutorial
    Explains poll(), non-blocking I/O
    More readable than Stevens for first pass




Sequence: Read these first for intuition,
then Stevens for depth:

Stevens Chapter 6 (I/O Multiplexing)

    Understand poll() semantics exactly
    Learn POLLIN, POLLOUT, POLLERR meanings
    Study the examples - trace by hand



...then RFCs for specification:

RFC 1945 Section 4 (HTTP Message format)

    Focus on the grammar (BNF notation)
    Note every MUST, SHOULD, MAY
    Understand what makes a valid request



...and Study 42 subject specification:

    What subset of HTTP/1.0 required?
    What must your config file support?
    What are the exact requirements?