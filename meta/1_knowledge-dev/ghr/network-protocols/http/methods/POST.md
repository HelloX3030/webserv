# POST

## ontology

POST is one of three HTTP methods webserv implements. a method is the
token in the request line that specifies what operation the client
requests the server perform on the identified resource.

```
POST /upload/ HTTP/1.1
^^^^
method token
```

the request line grammar (RFC 1945 §5.1):

```
Request-Line = Method SP Request-URI SP HTTP-Version CRLF
Method       = "GET" | "HEAD" | "POST" | extension-method
```


## semantics

RFC 1945 §8.3 defines POST:

> the POST method is used to request that the destination server accept
> the entity enclosed in the request as a new subordinate of the resource
> identified by the Request-URI.

unpacking this:

**"entity enclosed in the request"**: the request body. POST requests
carry a payload — the bytes following the blank line after the headers.

**"new subordinate of the resource"**: the server creates or modifies
something *under* the target resource. POST to `/articles/` might create
a new article within that collection. POST to `/upload/` might create a
new file within that directory.

**"identified by the Request-URI"**: the URI names the *handler*, not
necessarily the final location of the created resource.

RFC 1945 enumerates POST's intended functions:
- annotation of existing resources
- posting a message to a bulletin board, newsgroup, mailing list
- providing a block of data to a data-handling process (e.g. form submission)
- extending a database through an append operation

the critical insight: **POST semantics are server-defined**. the protocol
specifies how to *transmit* data; the server decides what *processing*
means. this is why webserv's POST behaviour depends on location
configuration — the spec deliberately leaves processing undefined.


## properties

**not safe**: POST has side effects. unlike GET, which only retrieves,
POST modifies server state.

**not idempotent**: sending the same POST twice may create two resources.
unlike PUT, which replaces a resource at a specific URI, POST appends or
creates new subordinates each time.

**carries a body**: POST requests have an entity-body. this is the data
being submitted.


## request structure

a POST request consists of:

```
POST /upload/ HTTP/1.1        ← request line
Host: localhost:8080          ← headers
Content-Type: multipart/form-data; boundary=----abc123
Content-Length: 1847
                              ← blank line (CRLF CRLF)
------abc123                  ← body begins
Content-Disposition: ...
...
------abc123--
```

**Content-Length**: specifies body size in bytes. required for the server
to know when the body ends. without it, the server cannot distinguish
"body continues" from "client finished sending".

**Content-Type**: specifies body encoding. tells the server how to
interpret the bytes. common values:
- `application/x-www-form-urlencoded` — key=value pairs, URL-encoded
- `multipart/form-data` — MIME multipart, required for file uploads
- `application/json` — JSON payload (APIs)
- `text/plain` — raw text


## body encodings

### application/x-www-form-urlencoded

the default for HTML forms without `enctype`. body is a single string:

```
name=john&age=30&city=london
```

keys and values are URL-encoded. `+` represents space. `%XX` represents
byte XX in hex. cannot transmit binary data — bytes outside ASCII must
be percent-encoded, bloating file content beyond usability.


### multipart/form-data

required for file uploads. the body is divided into MIME parts, each
separated by a boundary string declared in the Content-Type header.

```
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4

------WebKitFormBoundary7MA4
Content-Disposition: form-data; name="username"

john_doe
------WebKitFormBoundary7MA4
Content-Disposition: form-data; name="file"; filename="photo.jpg"
Content-Type: image/jpeg

<binary JPEG bytes>
------WebKitFormBoundary7MA4--
```

structure of each part:
- `--` + boundary string (delimiter)
- part headers (Content-Disposition, optional Content-Type)
- blank line
- part body (the actual data)

the final boundary has `--` appended: `------WebKitFormBoundary7MA4--`

**Content-Disposition**: identifies the form field.
- `name="fieldname"` — the HTML input's name attribute
- `filename="original.txt"` — present only for file inputs; the client's
  original filename

the server extracts the filename from Content-Disposition to name the
saved file. path traversal attacks (`../../etc/passwd`) are mitigated by
stripping directory components via `basename()`.


## client-server flow

1. **client constructs request**: browser builds POST from form submission.
   `<form method="POST" enctype="multipart/form-data">` triggers multipart
   encoding. browser generates boundary string, wraps each input field
   as a part.

2. **client sends**: TCP connection established, request bytes transmitted.

3. **server receives**: webserv's event loop detects readable socket.
   HttpRequestFrontend parses request line, headers, body.

4. **server dispatches**: runtime matches URI to location block. checks
   `allowed_methods`. if POST not permitted, returns 405.

5. **server processes**: action depends on location configuration:
   - CGI configured → body piped to CGI stdin
   - upload configured → body parsed, file written to upload_store
   - neither → undefined (webserv must decide: 405? 200 no-op?)

6. **server responds**: appropriate status code + headers + optional body.


## webserv POST behaviours

the 42 spec requires webserv to "accept file uploads from clients".
this manifests as two distinct POST handlers, selected by location config.


### CGI POST

triggered when location has `cgi_extension` + `cgi_path` configured.

```nginx
location /cgi-bin/ {
    cgi_extension .py;
    cgi_path /usr/bin/python3;
    allowed_methods GET POST;
}
```

behaviour:
- server forks, execs interpreter with CGI script
- request body written to child's stdin
- `Content-Length` passed as `CONTENT_LENGTH` env var
- `Content-Type` passed as `CONTENT_TYPE` env var
- CGI script reads stdin, processes data, writes response to stdout
- server reads child stdout, returns to client

the server does **not** interpret the body. it passes raw bytes to the
CGI process. parsing form data is the script's responsibility.


### upload POST

triggered when location has `upload_enable on` + `upload_store` configured.

```nginx
location /upload/ {
    upload_enable on;
    upload_store ./www/uploads;
    allowed_methods GET POST;
}
```

behaviour:
- server parses multipart body
- extracts file content and filename from Content-Disposition
- sanitises filename: `basename()` strips path components
- writes file bytes to `upload_store/filename`
- returns success response (200/201)

this is the "create file + write body to it" behaviour Lukas described.
it applies **only** when upload is configured for the location.


### neither configured

the spec does not define POST behaviour for locations without CGI or
upload directives. possible implementations:
- 405 Method Not Allowed (POST not meaningful here)
- 200 OK with no action (accept and discard)
- 501 Not Implemented

webserv should probably return 405 if POST reaches a static-only location,
since accepting data without processing it violates least surprise.


## security considerations

**path traversal**: malicious filename `../../etc/passwd` could write
outside upload_store. mitigate with `basename()` + absolute path
validation.

**unrestricted upload to executable path**: if uploaded files land in a
CGI-enabled location, an attacker uploads `evil.py`, then GETs it —
server executes attacker code. webserv prevents this structurally:
`upload_store` and `cgi_path` are separate directories.

**body size limits**: `client_max_body_size` prevents denial-of-service
via giant uploads exhausting disk or memory.


## references

- RFC 1945 §8.3 — POST method definition (HTTP/1.0)
- RFC 7578 — multipart/form-data specification
- RFC 2046 §5.1 — MIME multipart boundary syntax
- OWASP unrestricted file upload — CWE-434
