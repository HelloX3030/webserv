# file descriptors: int handles into kernel fd table
# stdin/stdout/stderr = 0/1/2
# open(), close(), dup(), dup2()
# per-process fd table → system-wide open file table → inodes

prerequisite to: sockets, epoll, webserv's reactor architecture
