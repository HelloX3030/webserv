## pre-implementation of Http Request Frontend by ghr


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
         my job
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
