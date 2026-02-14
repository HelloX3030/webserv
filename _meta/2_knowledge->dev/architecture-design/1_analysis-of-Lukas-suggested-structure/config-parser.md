## overview

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






## Config File Format: NGINX-Style

From specification:
"Inspired by NGINX server block syntax"


example config:

```cpp
server {
    listen 8080;
    server_name example.com www.example.com;
    client_max_body_size 1M;
    error_page 404 /errors/404.html;
    
    location / {
        root /var/www/html;
        index index.html index.htm;
        allowed_methods GET POST;
    }
    
    location /uploads/ {
        root /var/www/uploads;
        allowed_methods POST DELETE;
        client_max_body_size 10M;
    }
    
    location /cgi-bin/ {
        root /var/www/cgi-bin;
        cgi_extension .py;
        cgi_path /usr/bin/python3;
    }
}
```

No JSON. No custom format. NGINX-style blocks.





what I must build:

```cpp
struct Location {
    std::string path_prefix;  // "/api/"
    std::string root;
    std::vector<std::string> index_files;
    std::set<std::string> allowed_methods;
    bool autoindex;
    std::string cgi_extension;
    std::string cgi_path;
};

struct Server {
    std::string host;
    uint16_t port;
    std::vector<std::string> server_names;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;  // 404 → "/errors/404.html"
    std::map<std::string, Location> locations;  // "/api/" → Location config
};

class ConfigParser {
public:
    std::vector<Server> parse(const std::string& filepath);
    
private:
    std::vector<Token> tokenize(const std::string& content);
    Server parse_server_block(TokenIterator& it);
    Location parse_location_block(TokenIterator& it);
};
```


Tokeniser produces:

```cpp
enum TokenType { KEYWORD, STRING, NUMBER, LBRACE, RBRACE, SEMICOLON };

struct Token {
    TokenType type;
    std::string value;
};
```

**Parser state machine:**
```
START
  → see "server" → PARSE_SERVER_BLOCK
      → see "listen" → parse address:port
      → see "server_name" → parse names
      → see "location" → PARSE_LOCATION_BLOCK
          → see "root" → parse path
          → see "index" → parse filenames
          → see "}" → END_LOCATION
      → see "}" → END_SERVER
```