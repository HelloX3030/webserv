Separation of Concerns

WebServ namespace:

    Owns ALL servers
    Owns ALL connections
    Runs THE event loop
    Dispatches to appropriate Server based on fd mapping

Server class:

    Represents one server {} block from config
    Knows how to route requests
    Generates responses for its routes
    Owns its listening socket fds (created at init)

Connection class (missing):

    Represents one client connection
    Tracks state machine (READING, PARSING, PROCESSING, WRITING)
    Buffers (read/write)
    Associated with a Server (for routing)

Listener class:

    Currently just {int fd, int server_id}
    Questionable utility - fd could live directly in Server
    If kept: rename to ListenSocket for clarity




Before I can begin with config parser:

We need a single event loop


Connection tracking:

    Where will client Connection objects live?
    In Server? In WebServ namespace?


Listener class purpose:

    Why separate class vs. just int fd in Server?
    What's server_id for if Listener is owned by Server?


Define data structures:

    What does Server class actually contain?
    What goes in WebServ namespace?
    What does parser produce?


My config parser output must match what the event loop expects to consume.