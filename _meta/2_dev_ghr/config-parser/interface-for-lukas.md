ServerConfig (from Config.hpp) is the interface type — 
i.e. Server::parse() will consume a ServerConfig 
rather than the parser directly populating a Server 

`ConfigParser::parse()` returns `std::vector<ServerConfig>`. 
`Server::parse()` should calls this & store the result. 




How to populate Server from ServerConfig - options:

1. Server::parse(path) becomes a 2-step function:

```cpp
void Server::parse(const std::string& path)
{
    ConfigParser parser;
    ServerConfig cfg = parser.parse(path);  // your code produces this

    // Lukas copies fields from cfg into his Server
    this->host                = cfg.listen[0].host;
    this->port                = cfg.listen[0].port;
    this->server_names        = cfg.server_names;
    this->client_max_body_size = cfg.client_max_body_size;
    this->error_pages         = cfg.error_pages;
    this->locations           = cfg.locations;
    // etc.
}
```


2. Or more cleanly, store the whole ServerConfig as a member:

```cpp
// in Server.hpp
#include "classes/Config.hpp"

class Server {
    ServerConfig config;  // ← holds all parsed data
    std::vector<int> listen_fds;  // ← runtime state
    ...
};

void Server::parse(const std::string& path)
{
    ConfigParser parser;
    this->config = parser.parse(path)[0];  // single-server case
    // or store the whole vector if Server manages multiple configs
}
```

The second form is cleaner — it avoids re-listing every field when `ServerConfig` changes. 
Ask `this->config.locations` rather than `this->locations`. 



Note: also need to `#include "classes/Config.hpp"` in `Server.hpp`