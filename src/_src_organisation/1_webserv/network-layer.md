EPollHandler — abstract contract for event loop participants
Listener — accepts connections, dispatches to Connection factory
Connection — client fd lifecycle, owns HttpParser instance
socket primitives, I/O multiplexing
