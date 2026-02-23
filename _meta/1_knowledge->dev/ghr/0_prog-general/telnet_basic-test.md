telnet testing 
minimal viable requirements

basic socket infrastructure:

    socket created and bound to port
    listen() called
    accept() works (creates client fd)
    recv() reads bytes from client
    send() writes bytes to client
    close() cleans up

don't need:

    full HTTP parser (can echo back anything)
    config parser (hardcode port)
    CGI (static responses only)
    multiple connections (1 at a time ok initially)

simplest test:
```cpp
// pseudo: blocking echo server
int listen_fd = socket(...bindlisten on 8080...);
int client_fd = accept(listen_fd);
char buf[1024];
int n = recv(client_fd, buf, 1024);
send(client_fd, "HTTP/1.0 200 OK\r\n\r\nHello\n", 26);
close(client_fd);
```

telnet connects, you type, server responds.
validates basic socket flow.