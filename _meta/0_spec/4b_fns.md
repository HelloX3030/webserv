# allowed external fns

categorised by fundamental capability domain

## process identity & lifecycle
- execve
- fork
- waitpid
- kill
- signal

## descriptor abstraction
- open
- socket
- pipe
- socketpair
- kqueue
- epoll_create
- close
- dup
- dup2
- fcntl
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

## protocol state machines
- listen
- accept
- connect
- htons
- htonl
- ntohs
- ntohl
- setsockopt

## error & introspection
- errno
- strerror
- gai_strerror