# specification

## 1. invariants

### process stability
- server must not crash or terminate unexpectedly
  under any circumstances
- includes: memory exhaustion, invalid input, resource limits
- failed invariant compliance results in project failure

### non-blocking I/O architecture
- all I/O operations on sockets, pipes, and FIFOs
  must be non-blocking
- single `poll()` (or `select()`, `epoll()`, `kqueue()`)
  multiplexes all I/O operations
- multiplexer monitors both read and write events simultaneously
- never call `read()`/`recv()` or `write()`/`send()`
  on non-blocking descriptors without prior readiness notification
- exception: regular disk files exempt from multiplexer requirement
- `errno` inspection after read/write operations
  to adjust behavior is forbidden

### request lifecycle
- no client request may hang indefinitely
- timeout mechanism required
- server must handle client disconnections gracefully

### operational continuity
- server remains available under stress testing,
  invalid input, and client misbehavior
- resilience is mandatory across all operational scenarios

---

## 2. build system

### makefile
- required rules: `$(NAME)`, `all`, `clean`, `fclean`, `re`
- no unnecessary relinking

### compilation
- compiler: `c++`
- flags: `-Wall -Wextra -Werror`
- standard: `-std=c++98` (42 Heilbronn permits `-std=c++17`)

### language constraints
- prefer C++ standard library over C equivalents
- example: `<cstring>` over `<string.h>`
- C functions permitted when no C++ equivalent exists
- external libraries forbidden
- Boost libraries forbidden

---

## 3. executable interface

```
./webserv [configuration_file]
```

- executable built in program's repository root
- configuration file path optional
- if omitted: implementation-defined default location used

---

## 4. permitted system calls

- execve
- fork
- waitpid
- kill
- signal
- socket
- accept
- listen
- bind
- connect
- close
- open
- pipe
- socketpair
- dup
- dup2
- fcntl
- setsockopt
- getsockname
- read
- write
- send
- recv
- select
- poll
- epoll_create
- epoll_ctl
- epoll_wait
- kqueue
- kevent
- getaddrinfo
- freeaddrinfo
- getprotobyname
- htons
- htonl
- ntohs
- ntohl
- stat
- access
- chdir
- opendir
- readdir
- closedir
- errno
- strerror
- gai_strerror
- event multiplexer macros and helper functions (e.g. `FD_SET`)

### forbidden
- `execve()` except for CGI subprocess creation
- `fork()` except for CGI subprocess creation

---

## 5. HTTP protocol

### protocol version
- HTTP/1.1 subset
- HTTP/1.0 acceptable as reference point
- full RFC compliance not required

### required methods
- GET
- POST
- DELETE

### request processing

#### request line parsing
- method extraction
- URI parsing
- HTTP version identification

#### header parsing
- all standard headers
- `Content-Length` for body size determination
- `Host` for server selection

#### body handling
- support `Content-Length`-specified bodies
- chunked transfer encoding:
  server un-chunks before passing to CGI
- CGI receives EOF-terminated plain body stream

### response generation

#### status codes
- must be accurate per HTTP specification

#### headers
- `Content-Type`
- `Content-Length`
- additional headers as appropriate

#### error pages
- default error pages required
- custom error pages configurable

### browser compatibility
- must function with standard web browsers
- NGINX behavior serves as reference for edge cases
- account for HTTP version differences when comparing

### functional capabilities
- serve fully static websites
- accept file uploads from clients
- execute CGI scripts

---

## 6. configuration system

### format
- inspired by NGINX `server` block syntax
- plain text configuration file

### server-level directives

#### network binding
- define all `interface:port` pairs for listening
- support multiple servers serving different content

#### server identification
- server names (optional, for virtual hosting)
- virtual hosting out of scope but permitted

#### global policies
- default error page paths
- maximum client request body size

### location-level directives

#### route matching
- URL prefix matching
- no regex support required

#### method control
- list of accepted HTTP methods per route

#### redirection
- HTTP redirect configuration

#### resource resolution
- root directory for route
- example: route `/api` with root `/var/www`
  resolves `/api/users` to `/var/www/users`

#### directory behavior
- autoindex: enable/disable directory listing
- index files: default file(s) when route resolves to directory

#### file upload
- authorization flag
- storage path specification

#### CGI configuration
- file extension to CGI interpreter mapping
- CGI interpreter path

### deliverable requirements
- provide configuration files demonstrating all features
- include default files for testing and evaluation

---

## 7. CGI execution

### invocation mechanism
- fork-exec pattern: `fork()` followed by `execve()`
- triggered by file extension matching configuration

### execution context
- CGI process runs in correct directory
  for relative path file access

### environment variables
- REQUEST_METHOD
- CONTENT_LENGTH
- QUERY_STRING
- all data necessary to reconstruct full client request
- client arguments passed correctly

### communication protocol

#### input to CGI
- request body written to stdin
- for chunked requests:
  server un-chunks, CGI receives plain body
- EOF signals end of input

#### output from CGI
- read from stdout
- contains HTTP headers followed by body
- if no `Content-Length` header: EOF marks end of response

### minimum requirements
- support at least 1 CGI interpreter
- examples: php-cgi, Python, Perl

---

## 8. quality attributes

### availability
- withstand stress testing
- handle multiple concurrent clients
- maintain responsiveness under load

### error handling coverage

#### client errors (4xx)
- 400 Bad Request: malformed HTTP
- 404 Not Found: missing resource
- additional 4xx codes as appropriate

#### server errors (5xx)
- 500 Internal Server Error: unexpected server conditions
- 503 Service Unavailable: resource exhaustion
- additional 5xx codes as appropriate

### testing requirements

#### mandatory testing
- `telnet` for raw protocol verification
- standard web browsers

#### recommended testing
- automated test suites (Python, Go, etc.)
- NGINX behavioural comparison
- stress testing tools
- multiple concurrent client scenarios

---

## 9. deliverables

### source files
- makefile
- header files: `*.{h,hpp}`
- implementation files: `*.cpp`
- template files: `*.{tpp,ipp}`

### configuration assets
- configuration files demonstrating all mandatory features
- default error page files (if custom pages not configured)

### testing assets
- example configuration for evaluation
- test files demonstrating features

---

## 10. bonus extensions

### evaluation conditions
- assessed only if mandatory requirements fully satisfied
- incomplete mandatory implementation: bonus not evaluated

### cookie and session management
- HTTP cookie support
- session tracking mechanisms
- provide simple working examples

### multiple CGI types
- support for multiple different CGI interpreters

---

## 11. scope

### explicitly out of scope
- virtual hosting (permitted but not required)
- full HTTP/1.1 feature set (subset acceptable)
- regex in route matching

### reference implementation
- NGINX serves as behavioural reference
- compare headers and response behaviors
- account for HTTP version differences

---

## 12. study requirements

### mandatory reading
- HTTP protocol RFCs
- HTTP/1.0 suggested as reference point (not enforced)

### testing protocol
- test with `telnet` before submission
- test with NGINX for behavioural comparison
- browser testing required