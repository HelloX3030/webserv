# Config Parser — Data Model

Derived directly from the grammar (1_grammar.md).
Every field corresponds to a grammar rule.
Every type is justified by the value set of the terminal.

---

## Enumerations

```cpp
enum class HttpMethod {
    GET,
    POST,
    DELETE
};
```

methods_dir terminals are a fixed closed set.
Enum enforces validity at compile time, not runtime.

---

## Structs

### ListenAddress

```cpp
struct ListenAddress {
    std::string  host;  // default: "0.0.0.0"
    uint16_t     port;
};
```

Derived from host_port. Host is optional in the grammar —
bare port is valid. Default host assigned at parse time.
uint16_t matches the valid port range [1, 65535].

### Location

```cpp
struct Location {
    std::string                  root;
    std::vector<std::string>     index_files;
    std::set<HttpMethod>         allowed_methods;
    bool                         autoindex;
    std::string                  cgi_extension;
    std::string                  cgi_path;
    std::optional<size_t>        client_max_body_size;

    // TODO: pending directive name agreement with Lukas
    // bool          upload_enable;
    // std::string   upload_store;
    // uint16_t      return_code;
    // std::string   return_path;
};
```

client_max_body_size is std::optional<size_t> because it is
an override of the server-level default. Absent means inherit;
present means override. A sentinel value (e.g. 0) would
conflate two distinct states.

### Server

```cpp
struct Server {
    std::vector<ListenAddress>          listen;
    std::vector<std::string>            server_names;
    size_t                              client_max_body_size;
    std::map<uint16_t, std::string>     error_pages;
    std::map<std::string, Location>     locations;
};
```

listen is a vector: the grammar permits multiple listen_dir
within one server block.

error_pages maps status code to path.
uint16_t for the key: status codes are three-digit integers,
range [100, 599].

locations maps path prefix (string) to Location.
Path prefix is the argument to location_block in the grammar.

---

## Parser Output

```cpp
std::vector<Server> parse(const std::string& filepath);
```

config = server_block, { server_block }
maps directly to std::vector<Server>.

---

## Defaults

Applied at parse time when a directive is absent.

| Field                        | Default       |
|------------------------------|---------------|
| ListenAddress::host          | "0.0.0.0"     |
| Server::client_max_body_size | 1048576 (1M)  |
| Location::autoindex          | false         |
| Location::client_max_body_size | std::nullopt |
| Location::allowed_methods    | {GET,POST,DELETE} |

allowed_methods default — decision rationale:

2 options when methods_dir is absent from a location block:

- Option 1: default {GET} — silence means GET only.
  Any other method returns 405.
- Option 2: default {GET,POST,DELETE} — silence means
  all methods permitted.

This decision is internal to the parser. The runtime reads
whatever is in the field and enforces it blindly — it cannot
distinguish a parsed value from a default.

Choice: option 2. A missing directive is an omission, not a
restriction. Defaulting to restricted causes silent 405 failures
during evaluation when the evaluator omits the directive.
Permissive default is the safer choice for a school project.

---

## Notes

size terminal (grammar) uses suffixes k/K, m/M, g/G.
Stored post-expansion as bytes in size_t.
Expansion is a parse-time operation, not a validation step.

cgi_extension and cgi_path are stored as strings.
Semantic coupling between them (both must be set to activate
CGI on a route) is a validation constraint, not a type constraint.