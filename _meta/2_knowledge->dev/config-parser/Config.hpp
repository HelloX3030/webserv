#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

/*
Config.hpp — pure data types produced by the config parser.
No methods, no behaviour. Describes operator intent as extracted from
the config file. All other components consume these types; none are
depended upon here.

Does not include base/base.hpp. That header pulls in sockets, signals,
and POSIX I/O — none of which belong in a data type definition. The
dependency arrow is: Server.hpp → Config.hpp → stdlib only.
*/


/*
HttpMethod

The grammar's methods_dir terminals form a closed set: GET, POST, DELETE.
enum class encodes this closure at the type level — the compiler rejects
any value outside the set. std::set<HttpMethod> therefore guarantees at
compile time that allowed_methods contains only valid members.
*/

enum class HttpMethod
{
    GET,
    POST,
    DELETE
};


/*
ListenAddress

Derived from the host_port production in the grammar.
host is optional in the grammar (bare port is valid); when absent the
parser assigns the default "0.0.0.0" — all interfaces.
uint16_t matches the TCP port space [0, 65535]. The semantic constraint
[1, 65535] is enforced in the validator, not here.
*/

struct ListenAddress
{
    std::string host; // default: "0.0.0.0"
    uint16_t    port;
};


/*
Location

Derived from location_block and location_dir productions.

client_max_body_size is std::optional<size_t> because it overrides the
server-level default only when explicitly present. Absent means inherit
from server; present means override. A sentinel value (e.g. 0) would
conflate two distinct semantic states. optional makes the distinction
unambiguous at the type level.

allowed_methods defaults to {GET, POST, DELETE} when methods_dir is
absent from a location block. The parser initialises this default before
processing any directives in the block.

cgi_extension and cgi_path are either both set or both absent.
The grammar cannot express this coupling; the validator enforces it.

Upload and redirect directives are declared but commented out pending
directive name agreement with Lukas (see 1_grammar.md).
*/

struct Location
{
    std::string              root;
    std::vector<std::string> index_files;
    std::set<HttpMethod>     allowed_methods; // default: {GET, POST, DELETE}
    bool                     autoindex;       // default: false
    std::string              cgi_extension;
    std::string              cgi_path;
    std::optional<size_t>    client_max_body_size; // default: std::nullopt (inherit)

    // Pending directive name agreement with Lukas:
    // bool        upload_enable;
    // std::string upload_store;
    // uint16_t    return_code;
    // std::string return_path;
};


/*
ServerConfig

Derived from server_block and server_dir productions.
Named ServerConfig, not Server, to distinguish this pure data record from
Lukas's Server class. ServerConfig is passive: it holds operator intent.
Server is an actor: it has lifecycle and behaviour. The distinction is
ontological, and the names reflect it.

listen is a vector: the grammar permits multiple listen_dir within one
server block — one server may listen on multiple addresses and ports.

error_pages maps HTTP status code to path. uint16_t key: status codes
are three-digit integers in [100, 599]. Range enforced in validator.

locations maps path prefix to Location. std::map gives O(log n) lookup
by prefix, which the request dispatcher will use at runtime.

client_max_body_size at server level is a concrete size_t, not optional.
It is always set — either explicitly from the config or from the default
(1048576, 1M). It is the value Location inherits when its own field is
std::nullopt.
*/

struct ServerConfig
{
    std::vector<ListenAddress>      listen;
    std::vector<std::string>        server_names;
    size_t                          client_max_body_size; // default: 1048576 (1M)
    std::map<uint16_t, std::string> error_pages;
    std::map<std::string, Location> locations;
};