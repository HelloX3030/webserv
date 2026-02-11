## Accessible Learning Resources (Bridge to RFCs)
Before diving into Stevens, get intuition:

### MDN Web Docs: 
Web technology for developers - https://developer.mozilla.org/en-US/docs/Web

    HTTP: Hypertext Transfer Protocol - https://developer.mozilla.org/en-US/docs/Web/HTTP

    Glossary of web terms - https://developer.mozilla.org/en-US/docs/Glossary


### Julia Evans
https://jvns.ca/

Zines - https://wizardzines.com/

    Networking! ACK! - https://wizardzines.com/zines/networking/

    HTTP: Learn your browser's language




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