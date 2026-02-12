# allowed external fns

Categorised by fundamental capability domain

**Process Identity & Lifecycle**
- execve
- fork
- waitpid
- kill
- signal

**Descriptor Abstraction**
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

**Namespace & Address Resolution**
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

**Event Coordination**
- select
- poll
- epoll_ctl
- epoll_wait
- kevent

**Protocol State Machines**
- listen
- accept
- connect
- htons
- htonl
- ntohs
- ntohl
- setsockopt

**Error & Introspection**
- errno
- strerror
- gai_strerror