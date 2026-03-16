Routing vs error response — the fork

```
bytes arrive
    │
    v
HttpRequestFrontend::advance()
    │
    ├─ Complete ──────────────────────────────────────┐
    │                                                 │
    │       HttpRequest                               │
    │           │                                     │
    │           v                                     │
    │       Router::route()                           │
    │           │                                     │
    │           v                                     │
    │       HandlerDecision                           │
    │           │                                     │
    │    ┌──────┼──────┬──────────┐                   │
    │    │      │      │          │                   │
    │    v      v      v          v                   │
    │  Static  CGI  Redirect   Error                  │
    │  File              ↓         ↓                  │
    │    │         (3xx)    (4xx from routing:        │
    │    │                   404 no location,         │
    │    │                   405 method denied,       │
    │    │                   403 traversal blocked)   │
    │    │                         │                  │
    │    └─────────────────────────┼──────────────────┘
    │                              │
    │                              v
    │                      HttpResponseFrontend
    │                              │
    │                              v
    │                          bytes out
    │
    └─ Failed ────────────────────────────────────────┐
                                                      │
          error_code (400, 413, 501)                  │
              │                                       │
              v                                       │
          HttpResponseFrontend::error()               │
              │                                       │
              v                                       │
          bytes out ──────────────────────────────────┘
```

Key distinction:

```


```

The parse-error path bypasses Router entirely. The request is malformed — there's nothing to route.





www/ integration:

Both paths converge on config.error_pages:

```cpp
config.error_pages = {
    400 → "/errors/400.html",
    404 → "/errors/404.html",
    ...
}
```

These paths are relative to some implicit root
```cpp
// error_page 404 /errors/404.html;
// interpreted as: www/html/errors/404.html
std::string full_path = "www/html" + error_pages[404];
```
