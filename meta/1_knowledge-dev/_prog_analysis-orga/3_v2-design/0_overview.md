first principles rebuild for GNUnet server
# v2 design — overview

first-principles rebuild for GNUnet server.


---


## principles

1. separation of data and behaviour
2. pure transformations where possible
3. impurity isolated and explicit
4. single responsibility per module
5. no duplication of logic


---


## architecture

```
bytes in
    │
    ▼
HttpRequestFrontend              pure: bytes → HttpRequest | Error
    │
    ▼
Router                           pure: (HttpRequest, Config) → HandlerDecision
    │
    ▼
Handlers                         impure: HandlerDecision → HttpResponse
    │                                    (file I/O, CGI exec)
    ▼
serialize                        pure: HttpResponse → bytes
    │
    ▼
bytes out
```

runtime (Connection) orchestrates. each module has 1 job.


---


## types

### HttpRequest

```cpp
struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
};
```

pure data. output of RequestFrontend.


### HttpResponse

```cpp
struct HttpResponse
{
    uint16_t status;
    std::map<std::string, std::string> headers;
    std::string body;
};
```

pure data. output of handlers. input to serialize.


### HandlerDecision

```cpp
enum class HandlerType { StaticFile, CGI, Redirect, Error };

struct HandlerDecision
{
    HandlerType type;
    std::filesystem::path resolved_path;
    uint16_t error_code;                    // for Error type
    std::string interpreter;                // for CGI type
    std::string redirect_location;          // for Redirect type
};
```

Router output. handlers trust it — no re-validation.


---


## pure functions

### serialize

```cpp
std::string serialize(const HttpResponse& response);
```

`HttpResponse → bytes`. no side effects.


### Router::route

```cpp
HandlerDecision route(const HttpRequest& request, const ServerConfig& config);
```

`(HttpRequest, Config) → HandlerDecision`. no side effects.

owns:
- location matching (longest prefix)
- method permission check
- handler type determination (static, CGI, redirect, error)
- path resolution + traversal check

Router is the security boundary.


---


## impure functions

### handlers

```cpp
HttpResponse handle_static_file(const HandlerDecision& decision);
HttpResponse handle_cgi(const HandlerDecision& decision);
HttpResponse handle_redirect(const HandlerDecision& decision);
HttpResponse handle_error(const HandlerDecision& decision, const ServerConfig& config);
```

handlers do I/O. they receive resolved paths, trusted decisions.
no routing logic — that's Router's job.


---


## vs v1

| concern                | v1                              | v2                          |
|------------------------|---------------------------------|-----------------------------|
| routing                | duplicated in 3 handlers        | Router, single location     |
| response data          | HttpResponseBuilder (mutable)   | HttpResponse (immutable)    |
| serialization          | embedded in builder             | standalone pure function    |
| handler input          | (config, path, body?)           | HandlerDecision             |
| handler responsibility | routing + execution             | execution only              |
