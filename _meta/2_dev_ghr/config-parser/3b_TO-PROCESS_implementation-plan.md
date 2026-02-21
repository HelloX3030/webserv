# Why not NGINX codebase:

1. Complexity mismatch

NGINX handles:

  200+ directives across 50+ modules
  Dynamic module loading
  Complex inheritance (http → server → location)
  Configuration reloading without restart
  Conditional blocks, variables, expressions
  Embedded scripting languages

webserv needs:

  ~15 directives
  Simple two-level nesting (server → location)
  Parse once at startup

Studying NGINX code: 10,000 lines to understand 100 lines you need.

2. Implementation language/style

NGINX:

  Written in C with manual memory management
  Custom data structures (ngx_str_t, ngx_array_t, pools)
  Macro-heavy, platform abstraction layers
  Optimized for production performance

webserv:

  C++17 with STL
  Different memory model
  Different error handling

Copying NGINX patterns: Imports complexity you don't need.



#  Implement recursive descent parser


```cpp
class ConfigParser {
private:
    std::vector<Token> tokens;
    size_t pos;
    
    Token peek() { return tokens[pos]; }
    Token consume() { return tokens[pos++]; }
    bool match(TokenType type) { return peek().type == type; }
    
    // One function per grammar rule
    std::vector<Server> parse_config();
    Server parse_server_block();
    Location parse_location_block();
    void parse_listen_directive(Server& server);
    void parse_root_directive(Location& loc);
    // etc.
};
```

Grammar rule → Parser function. Direct correspondence.



Step 3: NGINX documentation (not code)

Read: NGINX directive documentation for semantics only.
Example - what does client_max_body_size mean?

Default value: 1m
Can use suffixes: k, m, g
Applies to request body size limit

Don't read: How NGINX parses it. You just need to know what it means.
Documentation URL: https://nginx.org/en/docs/http/ngx_http_core_module.html

Scan for:

Directive names (listen, server_name, root, etc.)
Value formats (port number, file path, size with suffix)
Default values
Validation rules (what's valid/invalid)

Ignore:

Module architecture
Configuration contexts (http, server, location inheritance)
Advanced features (regex locations, internal redirects, etc.)



Your implementation plan:
markdown
# Config parser implementation

## Phase 1: Tokenizer (lexer)


## Phase 2: Parser structure
```cpp
class ConfigParser {
private:
    std::vector tokens;
    size_t pos;
    
    // Helpers
    Token peek();
    Token consume();
    void expect(TokenType type);  // Throws if mismatch
    
    // Grammar rules (one per BNF rule)
    std::vector parse_config();
    Server parse_server_block();
    Location parse_location_block();
    
public:
    std::vector parse(const std::string& filepath);
};
```

## Phase 3: Implement grammar rules

**Example - parse_server_block():**
```cpp
Server ConfigParser::parse_server_block() {
    expect(KEYWORD); // "server"
    expect(LBRACE);
    
    Server server;
    // Set defaults
    server.host = "0.0.0.0";
    server.port = 80;
    server.client_max_body_size = 1048576; // 1M
    
    while (peek().type != RBRACE) {
        Token directive = consume();
        
        if (directive.value == "listen") {
            parse_listen_directive(server);
        } else if (directive.value == "server_name") {
            parse_server_name_directive(server);
        } else if (directive.value == "location") {
            Location loc = parse_location_block();
            server.locations[loc.path_prefix] = loc;
        } else {
            throw std::runtime_error("Unknown directive: " + directive.value);
        }
    }
    
    expect(RBRACE);
    return server;
}
```

**Each directive gets a helper:**
```cpp
void ConfigParser::parse_listen_directive(Server& server) {
    // "listen 8080;"
    Token port_token = consume();
    expect(SEMICOLON);
    
    server.port = std::stoi(port_token.value);
    
    if (server.port < 1 || server.port > 65535) {
        throw std::runtime_error("Invalid port: " + port_token.value);
    }
}
```

## Phase 4: Validation

After parsing, validate:
- Port in range [1, 65535]
- Paths exist (if checking filesystem)
- Methods are valid (GET, POST, DELETE only)
- Size suffixes parsed correctly (1M → 1048576)

## Phase 5: Error handling

Good error messages:
```
Error at line 12: Expected ';' after directive
Error at line 8: Invalid port number: 99999
Error at line 15: Unknown directive: listen2
Error: Duplicate location block: /api/
```

Track line numbers in Token struct for this.
What you DON'T need from NGINX:

Variable expansion ($host, $uri)
Regex locations (location ~ \.php$)
Conditional blocks (if ($request_method = POST))
Include directives (include mime.types;)
Upstream blocks (load balancing)
SSL configuration
Rewrite rules
Access control
Rate limiting
95% of NGINX features

Summary:
Don't study NGINX code - complexity trap, wrong language, wrong approach.
Do:

Write BNF grammar (I provided starting point)
Implement recursive descent parser (one function per rule)
Read NGINX docs for directive semantics only
Test incrementally (tokenizer → parser → validation)

You already understand the structure. Now execute the implementation.
Next step: Implement tokenizer. Test it. Then parser. Then validation. Linear progression.