What must parser produce?

```cpp
struct Server {
    std::string host;
    uint16_t port;
    std::map<std::string, Location> locations;
};

struct Location {
    std::string root;
    std::vector<std::string> index_files;
    std::set<std::string> allowed_methods;
    bool autoindex;
};
```

Parser is pure transformation: config_file_bytes → Server objects.

No I/O handling, no networking. Just lexing + parsing + validation.


Simplest approach:

    Read entire file into string
    Tokenize (whitespace-delimited)
    Recursive descent parser or state machine
    Build structs
    Validate (port in range, paths exist, etc.)

If config syntax is complex (nested blocks, directives), 
study NGINX config parser or write BNF grammar first.