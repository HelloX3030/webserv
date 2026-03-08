#pragma once

#include <cstdint>
#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

/*
Config.hpp — pure data types produced by the config parser.
No methods, no behaviour. Describes operator intent as extracted from
the config file. All other components consume these types; 
none are depended upon here.

Does not include base/base.hpp. That header pulls in sockets, signals,
and POSIX I/O — none of which belong in a data type definition. 
dependencies: Server.hpp → Config.hpp → stdlib only.
*/

/*
The grammar's methods_dir terminals form a closed set: GET, POST, DELETE.
enum class enforces membership at compile time - 
an unknown method cannot be represented, no runtime string comparison required.
*/
enum class HttpMethod
{
    GET,
    POST,
    DELETE
};

/* derived from the host_port grammar production.
host is optional — bare port (`listen 8080;`) is valid.
when absent, parser assigns default host "0.0.0.0".

`port` type:
must contain full TCP port space [0, 65535].
    2^8 = 256 < 65535   
    2^16 = 65536 > 65535
∴ uint16_t chosen (unsigned integer type w/ width 16 bits)
as minimal std width.

valid range enforced at parse time in parse_port() 
& confirmed in validator. */
struct ListenAddress
{
    std::string host; // default: "0.0.0.0"
    uint16_t    port;
};

/*
Derived from location_block and location_dir productions.
1 `Location` per location {} block within a server block.

`client_max_body_size` is std::optional<size_t>: it overrides the
server-level default when present. std::nullopt means inherit.
A sentinel value (e.g. 0) would conflate "not set" with "set to 0".

allowed_methods default: {GET, POST, DELETE} — all methods permitted
unless explicitly restricted. absent directive = no restriction.
alternative considered: default {GET} (silence means GET only).
rejected: too restrictive for an evaluation server where the evaluator
expects all methods unless explicitly limited. explicit restriction
via allowed_methods directive is the opt-in.

`cgi_extension` and `cgi_path` are semantically coupled: 
either both set or both absent. Validator enforces.

`upload_enable` / `upload_store`: upload_enable=true requires upload_store
to be non-empty. Validator enforces.

return_code is std::optional<uint16_t>: std::nullopt means no redirect
configured for this location. When set, return_path must be non-empty.
Validator enforces coupling. Valid range [300, 399] enforced at parse
time and confirmed in validator.
*/
struct Location
{
    std::string                 root;
    std::vector<std::string>    index_files;
    std::set<HttpMethod>        allowed_methods; // default: {GET, POST, DELETE}
    bool                        autoindex;       // default: false
    std::string                 cgi_extension;
    std::string                 cgi_path;
    std::optional<size_t>       client_max_body_size; // default: std::nullopt (inherit)
    bool                        upload_enable;
    std::string                 upload_store;
    std::optional<uint16_t>     return_code;
    std::string                 return_path;
};

/*
Derived from server_block and server_dir productions.

Named ServerConfig, not Server.
    ServerConfig is passive: it holds operator intent.
    Server class is operational, an actor: 
    has lifecycle & behaviour.

`listen` is a vector: the grammar permits multiple listen_dir within
1 server block — 1 server may listen on multiple addresses.

`error_pages` maps HTTP status code to path.
uint16_t key: status codes are non-negative, maximum 599, requiring
> 8 bits (2^8 = 256 < 599). uint16_t is the minimal standard
width that fits [100, 599]. Valid range enforced at parse time and
confirmed in validator.

`locations` maps path prefix to Location.
std::map: O(log n) prefix lookup used by the request dispatcher.

`client_max_body_size` is a concrete size_t, not optional. Always set
— either from config or from the default (1048576, 1M). This is the
value Location inherits when its own field is std::nullopt.
*/
struct ServerConfig
{
    std::vector<ListenAddress>      listen;
    std::vector<std::string>        server_names;
    size_t                          client_max_body_size; // default: 1048576 (1M)
    std::map<uint16_t, std::string> error_pages;
    std::map<std::string, Location> locations;
};

/*
display serialisation — Config.cpp

free functions, not methods: 
Config.hpp contracts "no methods, no behaviour". 
to_string is orthogonal to data definition —
it is observation of state, not state or behaviour itself.

<iosfwd> provides the std::ostream forward declaration.
a forward declaration is sufficient here: the signatures take
std::ostream& but do not construct, destruct, or access its members.
the full definition (<ostream>) lives in Config.cpp.

operator<< delegates to to_string — sgl rendering path,
2 call sites: stream output & string embedding.
*/
std::string   to_string(const ListenAddress& addr);
std::string   to_string(const Location& loc);
std::string   to_string(const ServerConfig& cfg);

std::ostream& operator<<(std::ostream& os, const ListenAddress& addr);
std::ostream& operator<<(std::ostream& os, const Location& loc);
std::ostream& operator<<(std::ostream& os, const ServerConfig& cfg);