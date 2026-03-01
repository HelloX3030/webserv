# HTTP request frontend — persistence support


## context

the frontend is a pure function: bytes in, structured request out.
it has no knowledge of connection lifecycle.
it exposes data that enables the runtime to make persistence
decisions.

the frontend can be "persistent-ready" before the runtime is.
the frontend doesn't need to know whether persistence is enabled —
it just exposes the data. the runtime makes the decision.


---


## requirements for persistence support


### 1. expose HTTP version

the request struct must contain the HTTP version string.

```cpp
std::string http_version;  /* "HTTP/1.1", "HTTP/1.0" */
```

why: HTTP/1.1 defaults to persistent connections.
HTTP/1.0 defaults to non-persistent.
the runtime needs this to compute the default behaviour.


### 2. expose Connection header

the request struct must expose all headers, including `Connection`.

```cpp
std::map<std::string, std::string> headers;
```

why: the client can override the default via:
- `Connection: close` (HTTP/1.1 client wants to close)
- `Connection: keep-alive` (HTTP/1.0 client wants to persist)


### 3. expose Content-Length

the request struct must expose Content-Length when present.

why: Content-Length determines where the request body ends.
without correct parsing, the frontend cannot identify request
boundaries. if boundaries are wrong, persistent connections fail —
the next request starts mid-body of the previous one.


### 4. correct request boundary detection

this is the critical constraint.

without persistent connections: EOF signals end-of-request.
with persistent connections: Content-Length signals end-of-body.

the frontend must:
- parse Content-Length header
- read exactly that many bytes for the body
- stop reading when body is complete
- not consume bytes belonging to the next request

if Content-Length parsing is correct now, persistent connections
work later. if it's approximate, they fail.


---


## header case normalisation


### the rule

RFC 1945 (HTTP/1.0), section 4.2:

> "Each header field consists of a name followed immediately by a
> colon (":"), a single space (SP) character, and the field value.
> Field names are case-insensitive."

RFC 7230 (HTTP/1.1), section 3.2:

> "Each header field consists of a case-insensitive field name
> followed by a colon..."

RFC 9110 (current HTTP semantics):

> "Field names are case-insensitive"

`Connection`, `connection`, `CONNECTION` are all the same header.


### implementation

two options:

1. normalise on parse: convert all header names to canonical form
   (e.g. lowercase) when building the headers map.
   lookup then uses exact match.

2. case-insensitive lookup: store headers as received, but
   provide accessor that compares case-insensitively.

recommendation: normalise to lowercase on parse.
simpler, single point of transformation, lookup is trivial.

```cpp
/* during header parsing */
std::string name = /* parsed header name */;
std::transform(name.begin(), name.end(), name.begin(), ::tolower);
headers[name] = value;
```

then lookup is simply:

```cpp
auto it = headers.find("connection");
```


---


## keepAlive() method


### purpose

pure derivation from already-parsed data.
no runtime coupling.
the runtime calls this after the response is sent to inform
its decision — but the method itself has no side effects.


### specification (pseudocode)

```
function keepAlive(request):
    if request.http_version == "HTTP/1.1":
        /* HTTP/1.1: persistent by default */
        /* close only if client explicitly requests */
        connection_header = request.headers.get("connection")
        if connection_header exists and connection_header == "close":
            return false
        return true

    if request.http_version == "HTTP/1.0":
        /* HTTP/1.0: not persistent by default */
        /* keep-alive only if client explicitly requests */
        connection_header = request.headers.get("connection")
        if connection_header exists and connection_header == "keep-alive":
            return true
        return false

    /* unknown version: close */
    return false
```

note: assumes headers are already normalised to lowercase.


### placement

method on HttpRequest struct.

```cpp
struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;

    bool keepAlive() const;
};
```


---


## HttpRequest struct — complete specification

```cpp
struct HttpRequest
{
    std::string method;
    std::string uri;
    std::string http_version;  /* "HTTP/1.1", "HTTP/1.0" */
    std::map<std::string, std::string> headers;
    std::string body;

    /*
    ** pure derivation from http_version and Connection header.
    ** returns true if client wants persistent connection.
    ** runtime combines this with response status to decide.
    */
    bool keepAlive() const;

    /*
    ** convenience accessor for Content-Length.
    ** returns -1 if header absent or malformed.
    */
    long contentLength() const;
};
```


---


## what the frontend does NOT do

- decide whether to keep connection open (runtime's job)
- track connection state (runtime's job)
- manage fd lifecycle (runtime's job)
- know whether persistence is enabled in config (doesn't need to)

the frontend's only persistence-related responsibility:
expose the data correctly.