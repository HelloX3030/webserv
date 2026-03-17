# architectural debt

coupling, duplication, conflation.


---


## 1. routing logic duplication

location: HttpMethods_get.cpp, HttpMethods_post.cpp, HttpMethods_delete.cpp

each handler contains identical routing logic:
- longest prefix match on config.locations
- method permission check
- path resolution
- traversal protection

this is config interpretation, not method execution.


---


## 2. data/serialization conflation

location: http/HttpResponseBuilder

```cpp
class HttpResponseBuilder {
    int status;
    std::string body;
    std::map<std::string, std::string> headers;

    std::string to_string() const;  // serialization embedded
};
```

clean separation would be:

```cpp
struct HttpResponse {               // pure data
    uint16_t status;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string serialize(const HttpResponse&);  // pure function
```


---


## 3. parsing/response conflation

location: HttpParser (placeholder)

```cpp
void add_buffer(...) {
    this->buffer.append(buffer, n);
    response = HttpResponseBuilder().to_string();  // generates response
}
```

parser produces response. will be fixed by HttpRequestFrontend.


---


## 4. net/ couples to http/

location: Connection owns HttpParser

```cpp
class Connection {
    HttpParser http_parser;  // http/ type in net/
};
```

layered design: net/ is protocol-agnostic.
Connection handles bytes; protocol layer interprets.

acceptable for webserv (HTTP only).
refactor for multi-protocol server (ghr's v2).


---


## 5. handler interface inconsistency

```cpp
HttpResponseBuilder http_get(config, path);
HttpResponseBuilder http_post(config, path, content);
HttpResponseBuilder http_delete(config, path);
```

POST takes body, others do not.
no common signature.

clean: all take `HttpRequest` or `HandlerDecision`.
