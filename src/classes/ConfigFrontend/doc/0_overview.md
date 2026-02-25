# configuration parser — overview


## essence

pure function:

```
bytes (config file) → std::vector<ServerConfig>
```

no I/O beyond file read. no networking. no side effects.
reads text, produces data structures.


---


## position in system

```
phase 1: PARSE     config file → std::vector<ServerConfig>
phase 2: INIT      configs → sockets, epoll registration
phase 3: RUN       event loop — accept, read, route, write
```

the parser owns phase 1 entirely.
hands off structured configuration to the runtime.

```
config file
    │
    v
ConfigParser::parse()
    │
    v
std::vector<ServerConfig>   ← parser's sole output
    │
    v
WebServ                     ← runtime takes over
```


---


## telos

produce a validated, complete, runtime-ready representation
of the operator's intent.

"runtime-ready": all fields populated, defaults applied,
semantic constraints verified. the runtime trusts the config
without further checking.


---


## what it is not

not part of the event loop.
not responsible for socket creation.
not responsible for request handling.
not a general-purpose config system:
    no variables, no inheritance, no includes.


---


## input / output contract

input:
    path to config file (NGINX-style syntax).

output:
    std::vector<ServerConfig>

    each ServerConfig holds:
        listen addresses (host, port)
        server_names
        client_max_body_size
        error_pages map
        locations map

    each Location holds:
        root, index files, allowed methods,
        autoindex, CGI config, upload config, return directive.

on error:
    throw std::runtime_error with line-numbered message.
    program must not start with malformed config.