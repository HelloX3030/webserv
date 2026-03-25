# URIs in webserv


## what the server receives

HTTP requests do not contain complete URIs.

a client connecting to `http://example.com/api/users?id=42` sends:
```
GET /api/users?id=42 HTTP/1.1
Host: example.com
```

the request-line contains `/api/users?id=42` — not the full URI.


---


## request-target forms

RFC 9112 §3.2 defines 4 forms for the request-target:

| form           | example                          | when used                    |
|----------------|----------------------------------|------------------------------|
| origin-form    | `/path/to/resource?query`        | most requests                |
| absolute-form  | `http://example.com/path`        | requests to proxies          |
| authority-form | `example.com:443`                | CONNECT method (tunnelling)  |
| asterisk-form  | `*`                              | OPTIONS for entire server    |

webserv implements origin-form only.
absolute-form, authority-form, asterisk-form → 400 Bad Request.


---


## origin-form structure
```
origin-form = absolute-path [ "?" query ]
absolute-path = 1*( "/" segment )
```

decomposition:
```
/api/users?id=42&sort=name
\________/ \____________/
    |            |
   path        query
```

**path**: begins with `/`. hierarchical segments.
**query**: begins with `?`. everything after until end of target.

no scheme. no authority. no fragment.

- scheme: implied by the connection (http vs https)
- authority: in the `Host` header
- fragment: never transmitted (client-side only)


---


## what HttpRequestFrontend stores

the `uri` field in `HttpRequest` contains the origin-form verbatim:
```cpp
request_.uri = "/api/users?id=42&sort=name";
```

no decomposition. no parsing of path vs query.
the frontend extracts the token; downstream logic interprets it.


---


## downstream responsibilities

**routing**: match path prefix against configured locations.
**query parsing**: split on `&`, decode `%HH`, extract key-value pairs.
**path resolution**: map URI path to filesystem path via location root.


---


## validation at the frontend

the frontend validates:

1. **starts with `/`**: origin-form requires absolute-path.
   violation → 400 Bad Request.

2. **no control characters**: URI must be printable.
   violation → 400 Bad Request.

the frontend does not:
- decode percent-encoding
- normalise path (remove `.` and `..`)
- split path from query
- validate path traversal attempts

these are downstream concerns, handled with full context
(configured roots, security policies).


---


## references

RFC 3986: URI generic syntax (see `0_general/`)
RFC 9112 §3.2: request-target
