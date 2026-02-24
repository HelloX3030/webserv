# allowed external fns

categorised by fundamental capability domain


## process identity & lifecycle
- execve
- fork
- waitpid
- kill
- signal


## descriptor abstraction

### descriptor creation
- open
- socket
- pipe
- socketpair
- kqueue
- epoll_create

### descriptor manipulation
- close
- dup
- dup2
- fcntl
- setsockopt

### data transfer
- read
- write
- send
- recv


## namespace & address resolution
- stat
- access
- chdir
- opendir
- readdir
- closedir
- getaddrinfo
- freeaddrinfo
- getprotobyname
- getsockname
- bind


## event coordination
- select
- poll
- epoll_ctl
- epoll_wait
- kevent


## connection lifecycle
- listen
- accept
- connect


## data representation
- htons
- htonl
- ntohs
- ntohl


## error & introspection
- errno
- strerror
- gai_strerror