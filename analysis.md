## Lukas' temporary implementations to create a running system, before ghr's implementation of http request frontend

interface:
```
void add_buffer(const char* buffer, ssize_t n);
bool response_ready() const;
std::string take_response();
```

Connection calls:
```
http_parser.add_buffer(buffer, n);
if (http_parser.response_ready())
    write_buffer = http_parser.take_response();
```

This conflates parsing with response generation.
Lukas' `HttpParser` *is* the response — it returns `"Hello"`.
This is a placeholder.


The separation ghr is establishing:
```
bytes → HttpRequest → route → handle → response bytes
         ^^^^^^^
         ghr's job
```

## when ghr has completed implementation of Http Request Frontend

Connection code will need to change from:
```
if (http_parser.response_ready())
    write_buffer = http_parser.take_response();
```

to:
```
ParseResult result = request_frontend.parse(buf, n);
if (result.status == ParseStatus::Complete)
    write_buffer = generate_response(result.request);  // routing + handling
```



# UPDATE:


my integration point:

Lukas's HttpParser is a placeholder that conflates parsing with response generation:

```cpp
void add_buffer(Connection&, const char*, ssize_t);  // appends bytes, sets hardcoded response
bool response_ready() const;                          // always true after any input
std::string take_response();                          // returns placeholder
```

Connection calls this in the event loop. my HttpRequestFrontend must replace HttpParser.

The interface contract I must conform to or negotiate:

Current:
```cpp
http_parser.add_buffer(connection, buffer, n);
if (http_parser.response_ready())
    write_buffer = http_parser.take_response();
```

my design:
```cpp
ParseResult result = request_frontend.advance(buf, n);
if (result.status == ParseStatus::Complete)
    // what goes here? Who routes? Who handles?
```

Without Router, the "Complete" path must feed directly into the existing
HttpMethods::http_get/post/delete functions with their embedded routing logic.


Q.
Given Lukas owns Connection and rejects the Router abstraction,
what exactly does my HttpRequestFrontend output, and who consumes it?


Options:
- my frontend outputs HttpRequest.
Connection calls http_get(config, request.uri) directly.
Routing duplication remains — not my problem.

- I negotiate a minimal interface change:
HttpRequest type agreed, Connection modified to call my advance().
