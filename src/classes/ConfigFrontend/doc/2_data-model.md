# Config Parser — Data Model

Derived directly from the grammar (1_grammar.md).
Every field corresponds to a grammar production.
Every type is justified by the value set of its terminal.

---

## Enumerations

```cpp
enum class HttpMethod {
    GET,
    POST,
    DELETE
};
```

`methods_dir` terminals are a fixed closed set known at compile time.
Enum enforces membership at compile time rather than runtime string
comparison. An unknown method cannot be represented.

---

## Structs

### ListenAddress

```cpp
struct ListenAddress {
    std::string  host;   // default: "0.0.0.0"
    uint16_t     port;
};
```

Derived from the `host_port` production. Host is optional in the
grammar — bare port (`listen 8080;`) is valid. When absent, the
parser assigns the default "0.0.0.0".

`uint16_t` for port: port numbers are non-negative (unsigned) and
their maximum valid value is 65535. 2^16 = 65536 > 65535, so 16 bits
suffice. 8 bits (max 255) do not. `uint16_t` is the minimal standard
integer width whose range contains [1, 65535].

The valid range [1, 65535] is enforced at parse time inside
`parse_port()` (immediate throw with line number) and confirmed
again in the validator. The type `uint16_t` alone does not enforce
this — it admits 0. That gap is closed by parse-time rejection:
the value is validated at the moment of construction, so an invalid
port never persists in a `ListenAddress`.

---

### Location

```cpp
struct Location {
    std::string                  root;
    std::vector<std::string>     index_files;
    std::set<HttpMethod>         allowed_methods;   // default: {GET, POST, DELETE}
    bool                         autoindex;          // default: false
    std::string                  cgi_extension;
    std::string                  cgi_path;
    std::optional<size_t>        client_max_body_size;
    bool                         upload_enable;      // default: false
    std::string                  upload_store;
    std::optional<uint16_t>      return_code;
    std::string                  return_path;
};
```

#### client_max_body_size

`std::optional<size_t>`, not `size_t`. Reason: it is an override of
the server-level default. 3 states are possible:

- Absent in config → inherit from `ServerConfig::client_max_body_size`.
- Present → override with this value.

If it were `size_t`, there is no way to distinguish "not set" from
"set to 0". A sentinel (0 = inherit) conflates 2 distinct states.
`std::optional` makes "absent" explicit in the type. `std::nullopt`
means inherit; any value means override.

#### allowed_methods

`std::set<HttpMethod>`: ordered, no duplicates, O(log n) membership
test. The grammar permits multiple method tokens in 1 directive;
set ensures duplicates in config are silently deduplicated.

Default is `{GET, POST, DELETE}` — all methods permitted when the
directive is absent. Alternative considered: default `{GET}` (silence
means GET only). Rejected: too restrictive for an evaluation server
where the evaluator expects all methods unless explicitly limited.

#### cgi_extension / cgi_path

Both strings. Semantically coupled: either both set or both absent.
One without the other is a configuration error detected in the
validator, not the parser (the grammar cannot express this constraint).

#### upload_enable / upload_store

`upload_enable` is a boolean flag. Default false: uploads are not
accepted unless explicitly enabled.

`upload_store` is the filesystem path where uploaded files are written.
It is a string. Semantically coupled to `upload_enable`: if
`upload_enable` is false, `upload_store` is ignored; if true,
`upload_store` must be non-empty. The validator enforces this.

#### return_code / return_path

`std::optional<uint16_t>` for `return_code`. Reason: same argument
as `client_max_body_size` — "not set" and "set to a value" are
distinct states. `std::nullopt` means no redirect is configured for
this location.

`return_path` is a string. Empty string when `return_code` is
`std::nullopt`.

Semantically coupled: either `return_code` has a value and
`return_path` is non-empty, or both are absent. Validator enforces.

`uint16_t` for the code value: redirect codes are HTTP status codes
in [300, 399]. Same sizing argument as `error_pages` key — maximum
value 399, `uint8_t` max 255 is insufficient, `uint16_t` max 65535
is sufficient. Valid range [300, 399] enforced at parse time in
`parse_return_dir()` and confirmed in the validator.

---

### ServerConfig

```cpp
/*
Derived from server_block and server_dir productions.
Named ServerConfig, not Server, to distinguish this pure data record
from the operational Server class. ServerConfig is passive: it holds
operator intent. Server is an actor: it has lifecycle and behaviour.

`listen` is a vector: the grammar permits multiple `listen_dir`
within one server block — one server may listen on multiple addresses.

`error_pages` maps HTTP status code to path.
Key type `uint16_t`: status codes are non-negative integers, maximum
599, requiring more than 8 bits. `uint16_t` is the minimal standard
width that fits the range [100, 599]. The valid range is enforced at
parse time (immediate throw on out-of-range) and confirmed in the
validator. `uint16_t` alone does not enforce it — this gap is
explicitly documented and closed at the enforcement site.

`locations` maps path prefix to Location.
std::map gives O(log n) lookup by prefix string, which the request
dispatcher uses at runtime to match incoming URI paths.

`client_max_body_size` at server level is a concrete `size_t`, not
optional. It is always set — either from config or from the default
(1048576, 1M). It is the value Location inherits when its own field
is `std::nullopt`.
*/
struct ServerConfig {
    std::vector<ListenAddress>      listen;
    std::vector<std::string>        server_names;
    size_t                          client_max_body_size; // default: 1048576 (1M)
    std::map<uint16_t, std::string> error_pages;
    std::map<std::string, Location> locations;
};
```

---

## Parser Output

```cpp
std::vector<ServerConfig> parse(const std::string& filepath);
```

`config = server_block, { server_block }` maps directly to
`std::vector<ServerConfig>`. One struct per server block in the file.

---

## Defaults

Applied at parse time when a directive is absent.
The struct is initialised with defaults before any directives are read.

| Field                              | Default               |
|------------------------------------|-----------------------|
| `ListenAddress::host`              | `"0.0.0.0"`           |
| `ServerConfig::client_max_body_size` | `1048576` (1M)      |
| `Location::autoindex`              | `false`               |
| `Location::client_max_body_size`   | `std::nullopt`        |
| `Location::allowed_methods`        | `{GET, POST, DELETE}` |
| `Location::upload_enable`          | `false`               |
| `Location::return_code`            | `std::nullopt`        |

Fields with no default (`root`, `cgi_extension`, `cgi_path`,
`upload_store`, `return_path`) are empty strings on construction.
The validator enforces mandatory presence where required.

---

## Type Enforcement Strategy

For fields where the primitive type (e.g. `uint16_t`) admits values
outside the valid domain (e.g. 700 as a status code), the enforcement
strategy is:

1. **Parse-time rejection**: the directive helper (`parse_error_page_dir`,
   `parse_port`, `parse_return_dir`) calls `std::stoi`, range-checks
   immediately, and throws with a line number if invalid.
   The illegal value never enters the struct.

2. **Validator confirmation**: the validator checks all constrained
   fields in the completed `std::vector<ServerConfig>`. Belt-and-
   suspenders — ensures no path through the parser bypassed step 1.

This 2-layer approach is chosen over a wrapper type (e.g. a
`HttpStatusCode` class with a throwing constructor) on the grounds
that wrapper types add boilerplate without proportionate benefit at
this project scale. The enforcement is explicit and its location is
documented here and in 3_plan.md.