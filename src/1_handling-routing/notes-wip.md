http/ protocol vs handlers
The handlers are doing too much. Look at HttpMethods_get.cpp:

location matching (Router's job)
method checking (Router's job)
path resolution (could be a utility)
file reading (file serving concern)
response construction (protocol)

These should be thin after Router exists. The question becomes: once Router decides "this is a static file request for /foo/bar.html", who does the file serving?
Options:

handlers/ at top level — handler behaviour is not HTTP protocol knowledge







The handlers currently contain Router logic. After Router exists:

Router: location matching, method dispatch, returns HandlerDecision
Handlers: execute the decision (read file, invoke CGI, build response)

This is the interface contract you need to agree on. Document the HandlerDecision type and who owns what.




complete request lifecycle:
```
┌─────────────────────────────────────────────────────────────────────────┐
│  NETWORK                                                                │
│  bytes on wire                                                          │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  net/Connection                                                         │
│  owns fd, read buffer, write buffer                                     │
│  calls HttpRequestFrontend::feed() with incoming bytes                  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  http/HttpRequestFrontend                                               │
│  bytes → HttpRequest                                                    │
│                                                                         │
│  output: HttpRequest { method, uri, version, headers, body }            │
│  OR:     error code (400, 413, 501)                                     │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  core/ (runtime orchestration)                                          │
│  receives complete HttpRequest                                          │
│  calls Router::route(request, config)                                   │
│  dispatches to appropriate handler                                      │
│  sends response via Connection                                          │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  http/Router                                                            │
│  (HttpRequest, ServerConfig) → HandlerDecision                          │
│                                                                         │
│  owns:                                                                  │
│    • location matching (longest prefix match against config)            │
│    • method checking (is this method allowed here?)                     │
│    • handler type determination (static file? CGI? redirect? error?)    │
│    • path resolution (root + uri → filesystem path, traversal check)    │
│                                                                         │
│  output: HandlerDecision { type, resolved_path, error_code, ... }       │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  handlers/                                                              │
│  execute the decision, produce response                                 │
│                                                                         │
│  StaticFileHandler:  read file at resolved_path → HttpResponse          │
│  CGIHandler:         fork/exec interpreter, capture output → response   │
│  RedirectHandler:    build 3xx response with Location header            │
│  ErrorHandler:       build error response, possibly from www/errors/    │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  http/HttpResponseFrontend                                              │
│  HttpResponse → bytes                                                   │
│                                                                         │
│  serializes: status line, headers, body → wire format                   │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  net/Connection                                                         │
│  writes bytes to fd                                                     │
└─────────────────────────────────────────────────────────────────────────┘
```



Handler Decision type:

```cpp
enum class HandlerType {
    StaticFile,    // serve a file
    CGI,           // execute CGI script
    Redirect,      // 3xx redirect
    Error          // 4xx/5xx error
};

struct HandlerDecision {
    HandlerType type;

    // StaticFile: the resolved, traversal-checked path
    std::filesystem::path file_path;

    // CGI: script path + interpreter
    std::filesystem::path script_path;
    std::string interpreter;  // from cgi_path directive

    // Redirect: code + target
    uint16_t redirect_code;
    std::string redirect_location;

    // Error: status code (handler looks up www/html/errors/{code}.html)
    uint16_t error_code;
};
```

Or use `std::variant` if you want type safety on which fields are valid.

---

**What Router decides, what handlers execute:**

| Concern | Who owns it | Why |
|---------|-------------|-----|
| Location matching | Router | Config interpretation |
| Method allowed? | Router | Config interpretation |
| Handler type | Router | Derived from location directives |
| Path resolution + traversal check | Router | Security boundary, config-dependent (root directive) |
| File I/O | Handler | Execution, not decision |
| CGI fork/exec | Handler | Execution |
| Response construction | Handler | Produces HttpResponse |
| Response serialization | HttpResponseFrontend | Protocol encoding |

---




**www/ integration:**

directory:

```
www/
├── cgi-bin/
│   └── hello.py
├── html/
│   ├── errors/
│   │   ├── 400.html
│   │   ├── 404.html
│   │   └── ...
│   ├── index.html
│   └── upload.html
└── uploads/
```

maps to config locations:

```conf
server {
    listen 8080;

    location / {
        root /path/to/www/html;
        index index.html;
    }

    location /cgi-bin/ {
        root /path/to/www/cgi-bin;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
    }

    location /uploads/ {
        root /path/to/www/uploads;
        allowed_methods POST DELETE;
        upload_enable on;
        upload_store /path/to/www/uploads;
    }
}
```


Router matches request URI → location → determines:

GET / → StaticFile, path=www/html/index.html
GET /cgi-bin/hello.py → CGI, script=www/cgi-bin/hello.py, interpreter=/usr/bin/python3
POST /uploads/file.txt → StaticFile (upload), path=www/uploads/file.txt
GET /nonexistent → Error, code=404


---


**Current violation in HttpMethods_get.cpp:**

```cpp
// THIS IS ROUTER'S JOB:
const Location *location = NULL;
for (auto it = config.locations.begin(); ...) {
    if (path.find(it->first) == 0) {
        // longest prefix match
    }
}
if (location->allowed_methods.count(HttpMethod::GET) == 0) {
    return HttpResponse(405);
}
auto safe = utils::resolve_path(base, relative);

// THIS IS HANDLER'S JOB:
std::ifstream file(file_path.c_str(), std::ios::binary);
buffer << file.rdbuf();
res.set_body(buffer.str());
```

After refactor, handler becomes:

```cpp
HttpResponse handle_static_file(const HandlerDecision& decision) {
    // Router already resolved path, checked traversal, confirmed method
    std::ifstream file(decision.file_path, std::ios::binary);
    if (!file.is_open()) {
        return HttpResponse(500);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    HttpResponse response(200);
    response.set_body(buffer.str());
    response.set_content_type(decision.file_path);
    return response;
}
```



The handler trusts the decision. It doesn't re-validate. Router is the security boundary.

communication to Lukas — the action items:

Routing logic extraction: location matching, method checking, path resolution move from handlers to Router
Handler interface change: handlers receive HandlerDecision, not raw (config, path)
www/ config: have config files point to www/ subdirectories
Error pages: ErrorHandler serves from www/html/errors/
