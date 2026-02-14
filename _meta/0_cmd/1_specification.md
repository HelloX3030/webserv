# Webserv: HTTP Server Specification

## 1. Invariants

### Process Stability
- Server must not crash or terminate unexpectedly under any circumstances
- Includes: memory exhaustion, invalid input, resource limits
- Failed invariant compliance results in project failure

### Non-Blocking I/O Architecture
- All I/O operations on sockets, pipes, and FIFOs must be non-blocking
- Single `poll()` (or `select()`, `epoll()`, `kqueue()`) multiplexes all I/O operations
- Multiplexer monitors both read and write events simultaneously
- Never call `read()`/`recv()` or `write()`/`send()` on non-blocking descriptors 
without prior readiness notification
- Exception: regular disk files exempt from multiplexer requirement
- `errno` inspection after read/write operations to adjust behavior is forbidden

### Request Lifecycle
- No client request may hang indefinitely
- Timeout mechanism required
- Server must handle client disconnections gracefully

### Operational Continuity
- Server remains available under stress testing, invalid input, and client misbehavior
- Resilience is mandatory across all operational scenarios

## 2. Build System

### Makefile
- Required rules: `$(NAME)`, `all`, `clean`, `fclean`, `re`
- No unnecessary relinking

### Compilation
- Compiler: `c++`
- Flags: `-Wall -Wextra -Werror`
- Standard: `-std=c++98` (42 Heilbronn permits `-std=c++17`)

### Language Constraints
- Prefer C++ standard library over C equivalents
- Example: `<cstring>` over `<string.h>`
- C functions permitted when no C++ equivalent exists
- External libraries forbidden
- Boost libraries forbidden

## 3. Executable Interface

```
./webserv [configuration_file]
```

- Configuration file path optional
- If omitted: implementation-defined default location used

## 4. Permitted System Calls

### Process Management
- execve
- fork
- waitpid
- kill
- signal

### File Descriptors
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

### I/O Operations
- read
- write
- send
- recv

### Event Multiplexing
- select
- poll
- epoll_create
- epoll_ctl
- epoll_wait
- kqueue
- kevent

### Address Resolution
- getaddrinfo
- freeaddrinfo
- getprotobyname

### Byte Order Conversion
- htons
- htonl
- ntohs
- ntohl

### Filesystem Operations
- stat
- access
- chdir
- opendir
- readdir
- closedir

### Error Handling
- errno
- strerror
- gai_strerror

### Additional Permissions
- Event multiplexer macros and helper functions (e.g., `FD_SET`)

### Forbidden
- `execve()` except for CGI subprocess creation
- `fork()` except for CGI subprocess creation

## 5. HTTP Protocol

### Protocol Version
- HTTP/1.1 subset
- HTTP/1.0 acceptable as reference point
- Full RFC compliance not required

### Required Methods
- GET
- POST
- DELETE

### Request Processing

#### Request Line Parsing
- Method extraction
- URI parsing
- HTTP version identification

#### Header Parsing
- All standard headers
- `Content-Length` for body size determination
- `Host` for server selection

#### Body Handling
- Support `Content-Length`-specified bodies
- Chunked transfer encoding: server un-chunks before passing to CGI
- CGI receives EOF-terminated plain body stream

### Response Generation

#### Status Codes
- Must be accurate per HTTP specification
- Minimum required:
  - 200 OK
  - 400 Bad Request
  - 404 Not Found
  - 500 Internal Server Error
  - 503 Service Unavailable

#### Headers
- `Content-Type`
- `Content-Length`
- Additional headers as appropriate

#### Error Pages
- Default error pages required
- Custom error pages configurable

### Browser Compatibility
- Must function with standard web browsers
- NGINX behavior serves as reference for edge cases
- Account for HTTP version differences when comparing

### Functional Capabilities
- Serve fully static websites
- Accept file uploads from clients
- Execute CGI scripts

## 6. Configuration System

### Format
- Inspired by NGINX `server` block syntax
- Plain text configuration file

### Server-Level Directives

#### Network Binding
- Define all `interface:port` pairs for listening
- Support multiple servers serving different content

#### Server Identification
- Server names (optional, for virtual hosting)
- Virtual hosting out of scope but permitted

#### Global Policies
- Default error page paths
- Maximum client request body size

### Location-Level Directives

#### Route Matching
- URL prefix matching
- No regex support required

#### Method Control
- List of accepted HTTP methods per route

#### Redirection
- HTTP redirect configuration

#### Resource Resolution
- Root directory for route
- Example: route `/api` with root `/var/www` 
resolves `/api/users` to `/var/www/users`

#### Directory Behavior
- Autoindex: enable/disable directory listing
- Index files: default file(s) when route resolves to directory

#### File Upload
- Authorization flag
- Storage path specification

#### CGI Configuration
- File extension to CGI interpreter mapping
- CGI interpreter path

### Deliverable Requirements
- Provide configuration files demonstrating all features
- Include default files for testing and evaluation

## 7. CGI Execution

### Invocation Mechanism
- Fork-exec pattern: `fork()` followed by `execve()`
- Triggered by file extension matching configuration

### Execution Context
- CGI process runs in correct directory for relative path file access

### Environment Variables
- REQUEST_METHOD
- CONTENT_LENGTH
- QUERY_STRING
- All data necessary to reconstruct full client request
- Client arguments passed correctly

### Communication Protocol

#### Input to CGI
- Request body written to stdin
- For chunked requests: server un-chunks, CGI receives plain body
- EOF signals end of input

#### Output from CGI
- Read from stdout
- Contains HTTP headers followed by body
- If no `Content-Length` header: EOF marks end of response

### Minimum Requirements
- Support at least one CGI interpreter
- Examples: php-cgi, Python, Perl

## 8. Quality Attributes

### Availability
- Withstand stress testing
- Handle multiple concurrent clients
- Maintain responsiveness under load

### Error Handling Coverage

#### Client Errors (4xx)
- 400 Bad Request: malformed HTTP
- 404 Not Found: missing resource
- Additional 4xx codes as appropriate

#### Server Errors (5xx)
- 500 Internal Server Error: unexpected server conditions
- 503 Service Unavailable: resource exhaustion
- Additional 5xx codes as appropriate

### Testing Requirements

#### Mandatory Testing
- `telnet` for raw protocol verification
- Standard web browsers

#### Recommended Testing
- Automated test suites (Python, Go, etc.)
- NGINX behavioral comparison
- Stress testing tools
- Multiple concurrent client scenarios

## 9. Deliverables

### Source Files
- Makefile
- Header files: `*.{h,hpp}`
- Implementation files: `*.cpp`
- Template files: `*.{tpp,ipp}`

### Configuration Assets
- Configuration files demonstrating all mandatory features
- Default error page files (if custom pages not configured)

### Testing Assets
- Example configuration for evaluation
- Test files demonstrating features

## 10. Bonus Extensions

### Evaluation Conditions
- Assessed only if mandatory requirements fully satisfied
- Incomplete mandatory implementation: bonus not evaluated

### Cookie and Session Management
- HTTP cookie support
- Session tracking mechanisms
- Provide simple working examples

### Multiple CGI Types
- Support for multiple different CGI interpreters

## 11. Scope

### Explicitly Out of Scope
- Virtual hosting (permitted but not required)
- Full HTTP/1.1 feature set (subset acceptable)
- Regex in route matching

### Reference Implementation
- NGINX serves as behavioral reference
- Compare headers and response behaviors
- Account for HTTP version differences

## 12. Study Requirements

### Mandatory Reading
- HTTP protocol RFCs
- HTTP/1.0 suggested as reference point (not enforced)

### Testing Protocol
- Test with `telnet` before submission
- Test with NGINX for behavioral comparison
- Browser testing required