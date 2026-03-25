# product vs process


## the separation

`HttpRequest` is the product.
`HttpRequestFrontend` is the process that produces it.

the product is data — the structured result.
the process is computation — the transformation that yields data.


---


## why separate?

HttpRequest flows downstream: routing, handlers, CGI, response generation.
these consumers need the result. they do not need parsing machinery.

HttpRequestFrontend's state is transient: buffer, phase, partial fields.
this state exists only during parsing. once Complete, it is irrelevant.

HttpRequest's fields are persistent: method, uri, headers, body.
this is the interface between parsing and execution.

separating them:
- clarifies ownership: frontend owns transient state, request owns result
- enables handoff: Connection passes HttpRequest downstream, not the frontend
- permits reset: frontend clears transient state for next request;
  the previous HttpRequest continues its lifecycle independently


---


## analogy

a compiler has intermediate representation during compilation.
the output is an executable.

no one ships the IR. the executable is the product.

HttpRequestFrontend is the compiler. HttpRequest is the executable.


---


## in code
```cpp
struct HttpRequestFrontend
{
    // transient: buffer_, phase_, body_remaining_, ...
    // produces:
    HttpRequest request_;
};

struct HttpRequest
{
    // persistent: method, uri, http_version, headers, body
    // consumed by: routing, handlers, CGI
};
```

Connection owns HttpRequestFrontend.
Connection hands HttpRequest to the executor.
the frontend is never seen downstream.
