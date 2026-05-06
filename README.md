*this project has been created as part of the 42 curriculum by `lseeger`, `go-donne`*


## description

`webserv` is an HTTP/1.1 server written in C++, configured via an NGINX-inspired configuration file. 
it serves static content, accepts file uploads, and executes CGI scripts.

a single `epoll` loop multiplexes all socket I/O. every socket, pipe, and CGI stdio descriptor
is non-blocking; no operation ever blocks the process. the server remains available under
stress, invalid input, and client misbehaviour.

the server targets Linux. I/O multiplexing uses epoll(7), a Linux-specific kernel facility;
the program does not build on kernels lacking it (BSD, macOS, illumos).

architecturally, the program is a pipeline of clearly separated frontends and actors:

    config frontend      →  ServerConfig[]
    Listener             →  binds, accepts → Connection
    epoll loop           →  drives Connection I/O readiness
    HttpRequestFrontend  →  bytes → HttpRequest
    Router               →  (HttpRequest, ServerConfig[]) → (Server, Location)
    HttpMethods          →  dispatches GET / POST / DELETE; routes CGI-targeted
                            requests through the CGI handler
    HttpResponseBuilder  →  HttpResponse → bytes


---


## instructions

### build

    make             # release
    make debug       # -g, DEBUG preprocessor flag, separate binary
    make leaks       # instrumented for valgrind / leak checking
    make re          # rebuild from clean

the Makefile produces `./webserv` in the repository root. 
compilation uses `c++ -Wall -Wextra -Werror -std=c++17`.

### run

    ./webserv                         # uses default config
    ./webserv config/valid/full.conf  # explicit config path

the configuration file argument is optional. if omitted, the server loads an
implementation-defined default configuration.

### configuration examples

`config/valid/` contains configurations demonstrating all mandatory features plus the bonuses:

    default.conf        minimal working server
    full.conf           all directives, all features exercised
    multi-server.conf   multiple servers on multiple ports
    redirect.conf       HTTP redirection
    uploads.conf        file upload handling
    cgi-python.conf     CGI with Python interpreter
    cgi-bash.conf       CGI with Bash interpreter
    cookies.conf        cookies and session tracking (bonus)
    tester.conf         configuration for the 42 webserv tester

the `valid/` subdirectory reflects a development-time distinction: a parallel `invalid/`
subdirectory held deliberately malformed configuration files used to test the config frontend's
rejection paths. only the valid configurations are retained for submission.

### testing

    make test                # full Python integration suite
    make test-leaks          # integration suite under valgrind
    make siege-test          # stress testing via siege

individual test targets are listed under `.PHONY` in the Makefile.


`siege` is a command-line HTTP load-testing utility — it opens many concurrent connections
to a target server, issues requests at a chosen rate or pattern, and reports throughput,
response times, and failure rates. it is generally used for sustained-pressure testing:
verifying that a server handles concurrent traffic without crashing, leaking, deadlocking,
or returning malformed responses. for webserv, siege exercises the epoll loop and connection
lifecycle under load, surfacing failure modes that single-request integration tests cannot.
correctness of HTTP semantics and adversarial-input handling are tested separately by the
Python integration suite; siege's role is purely behaviour under concurrent load.


---


## sources

### specifications

- 42 `webserv` subject and evaluation sheet (primary specification and source of truth)
- RFC 7230 — HTTP/1.1: message syntax and routing
- RFC 7231 — HTTP/1.1: semantics and content
- RFC 3875 — the Common Gateway Interface (CGI) version 1.1

the RFCs served as reference during implementation — consulted to resolve specific ambiguities
in header handling, body framing, and CGI I/O — rather than as documents read in full.

### references

- NGINX source and documentation (behavioural reference for edge cases)
- Beej's Guide to Network Programming (socket API)
- `man 7 epoll`, `man 2 epoll_ctl`, `man 2 epoll_wait`

### AI usage

#### lseeger

ChatGPT (OpenAI) was used primarily to understand the project scope and how to approach it, and more
intensively for the development of the test suite.

#### go-donne

Claude (Anthropic) was used throughout development as a collaborative thinking partner.
use included protocol clarification, refining understanding of language processing within
config and http request frontends, documentation drafting, and targeted code generation.
all AI contributions were actively reviewed, critiqued, and frequently rewritten by hand
against primary sources — RFCs, NGINX behaviour, reference texts, and first-principles reasoning.