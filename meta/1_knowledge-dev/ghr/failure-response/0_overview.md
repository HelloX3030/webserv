# failure response — overview

a program encounters conditions it cannot satisfy: a file is absent,
input violates the grammar, a socket closes unexpectedly. these are
failures — states in which the program cannot fulfil its contract.

failure response is the complete system for handling them:
detecting, propagating, handling, and communicating failures.
it is not a single mechanism but a pipeline with distinct phases.


---


## the 4 phases

**detection**: identify that a failure condition exists.
a syscall returns -1. a value falls outside a valid range.
a required field is absent. the program discovers it cannot proceed.

**propagation**: carry the failure from the detection site to the
site equipped to handle it.
in C++: `throw`. the exception unwinds the call stack until a
matching `catch` is found. no intermediate layer need know.

**handling**: decide what the program does in response.
for fatal failures (bad config, unrecoverable I/O): log and exit.
for recoverable failures (bad HTTP request): send an error response,
close the connection, continue serving others.

**communication**: make the failure visible — to the operator,
to the programmer, to a monitoring system.
this is the log system's role. it is not propagation and not
handling. it is output: translating a failure event into human-
or machine-readable form.

the phases are sequential and separable. the log system participates
only in the last phase. a `throw` site has no knowledge of how its
failure will be communicated — that is determined at the catch site.


---


## program-wide contract in webserv

```cpp
// main.cpp
try {
    WebServ::parse(argc, argv);
    WebServ::init();
    WebServ::run();
}
catch (const std::exception& e) {
    log::log(WEB_SERV, e.what(), log::LogType::ERROR);
    WebServ::quit();
    return 1;
}
```

all components that encounter fatal failures throw `std::runtime_error`.
`main` catches once. the message string is the sole carrier of
diagnostic information. no error codes. no per-layer catch-and-rethrow.
no out-parameters.

this is a deliberate design decision, not a default: it places the
responsibility for message quality at the throw site, where the
context is richest. by the time the exception reaches `main`, only
the message survives — so the message must be sufficient.

recoverable failures (bad HTTP request, unexpected client disconnect)
do not throw. they are handled locally within the event loop and
communicated via log calls at the handling site.


---


## the 2 failure classes

**fatal**: the program cannot continue.
examples: config file absent, config structurally invalid, epoll
setup failed.
response: propagate to `main`, log as ERROR to stderr, exit.

**recoverable**: the current operation fails, the program continues.
examples: malformed HTTP request, client disconnects mid-transfer,
CGI process error.
response: handle locally, send appropriate HTTP error response if
applicable, log as ERROR or WARN, continue event loop.

the distinction determines propagation strategy:
fatal failures use `throw` because they must escape deep call chains.
recoverable failures are handled in-place because the program
continues and the event loop must not be interrupted.


---


## where the log system connects

communication is the final phase. the log system is its implementation.

at the fatal catch site: `log::log(..., LogType::ERROR)` routes to
stderr — the diagnostic stream. the operator sees what failed.

at recoverable handling sites: `log::log(..., LogType::DEFAULT)` or
`LogType::ERROR` routes to stdout or stderr depending on severity.
the programmer, in a debug build, sees the event stream.

the log system does not determine what is a failure. it does not
propagate anything. it receives a message already constructed at
the throw or handling site, and outputs it. the intelligence is in
the message — the log system is the channel.


---


## message discipline

the message is the only output that survives to the operator.
it must be self-sufficient:

- name the subsystem: `[config]`, `[http]`
- locate the failure: line number (parse-time), field value
  (validate-time), fd or address (runtime)
- state what was expected and what was found

see: observability/webserv_log-sys/2_error-formats.md


---


## propagation mechanism

`std::runtime_error`, the exception hierarchy, 
and the throw/catch pattern in detail:

see: failure-response/1_propagation.md