# resource limits: soft vs hard
# RLIMIT_NOFILE, RLIMIT_AS, RLIMIT_STACK, etc.
# getrlimit(), setrlimit() syscalls
# ulimit shell built-in: query/set for session
# inheritance: child inherits parent's limits
# /etc/security/limits.conf for persistence

prerequisite to: valgrind usage, server hardening (RLIMIT_NOFILE for max connections)
