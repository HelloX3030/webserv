# Config Parser — Implementation Plan

---

## Pipeline

The parser is a linear pipeline of 4 phases.
Each phase has a single input type and a single output type.
Failure at any phase throws with a precise error message.

```
std::string (filepath)
    |
    | Phase 1: File read
    v
std::string (raw content)
    |
    | Phase 2: Lexer
    v
std::vector<Token>
    |
    | Phase 3: Parser
    v
std::vector<Server>  (fields populated, defaults applied)
    |
    | Phase 4: Validation
    v
std::vector<Server>  (semantically verified)
```

---

## Phase 1 — File Read

Read entire file into a std::string.
Strip comments: from `#` to end of line, replace with whitespace.
Whitespace preservation keeps line numbers accurate for errors.

---

## Phase 2 — Lexer

### Input / Output

```
std::string → std::vector<Token>
```

### Token

```cpp
enum class TokenType {
    KEYWORD,
    STRING,
    NUMBER,
    LBRACE,
    RBRACE,
    SEMICOLON,
    END_OF_FILE
};

struct Token {
    TokenType   type;
    std::string value;
    size_t      line;
};
```

line is carried on every token. It is the only mechanism
for producing precise error messages in later phases.

### Rules

- `{`  → LBRACE
- `}`  → RBRACE
- `;`  → SEMICOLON
- digit-leading sequence → NUMBER
- anything else          → KEYWORD or STRING

KEYWORD and STRING are the same token type at the lexer level.
The parser assigns meaning based on position in the grammar,
not on the token type alone. 
A path like `/var/www` and a directive name like `root` 
are both STRING/KEYWORD — the
parser knows which is which from context.

### Lexer does not

- validate values (port range, path existence, etc.)
- distinguish directive names from other strings
- interpret size suffixes

---

## Phase 3 — Parser

### Structure

Recursive descent. 1 function per grammar production rule.
The grammar is the specification; the code is a direct
translation of it.

```cpp
class ConfigParser {
    std::vector<Token> tokens;
    size_t             pos;

    Token  peek();
    Token  consume();
    Token  expect(TokenType type);  // consume or throw
    bool   at(const std::string& value);

    std::vector<Server>  parse_config();
    Server               parse_server_block();
    void                 parse_server_dir(Server& s);
    void                 parse_listen_dir(Server& s);
    void                 parse_server_name_dir(Server& s);
    void                 parse_body_size_dir(size_t& out);
    void                 parse_error_page_dir(Server& s);
    Location             parse_location_block();
    void                 parse_location_dir(Location& loc);
    void                 parse_root_dir(Location& loc);
    void                 parse_index_dir(Location& loc);
    void                 parse_methods_dir(Location& loc);
    void                 parse_autoindex_dir(Location& loc);
    void                 parse_cgi_ext_dir(Location& loc);
    void                 parse_cgi_path_dir(Location& loc);

    size_t               parse_size(const Token& t);
    uint16_t             parse_port(const Token& t);
    ListenAddress        parse_host_port(const Token& t);

public:
    std::vector<Server>  parse(const std::string& filepath);
};
```

### Defaults applied here

When a directive is absent, the parser sets the default value
as specified in 2_data-model.md. The struct is initialised
with defaults before any directives are parsed.

### Error format

```
[config] line <N>: <message>
```

Examples:

```
[config] line 12: expected ';'
[config] line 8:  unknown directive 'listen2'
[config] line 5:  expected '{' after 'server'
```

---

## Phase 4 — Validation

Runs after the full std::vector<Server> is built.
Checks semantic constraints that the grammar cannot express.

### Mandatory fields

| Level    | Field    | Rule                                 |
|----------|----------|--------------------------------------|
| Server   | listen   | at least one ListenAddress required  |
| Server   | locations| at least one location block required |
| Location | root     | required                             |

### Value range checks

| Field              | Constraint          |
|--------------------|---------------------|
| ListenAddress::port| [1, 65535]          |
| error_pages key    | [100, 599]          |
| client_max_body_size | > 0               |

### Semantic coupling

- cgi_extension and cgi_path: either both set or both absent.
  One without the other is an error.

### Duplicate checks

- Duplicate listen address+port pair across servers:
  valid (spec allows multiple servers on same port for
  virtual hosting), but note it for awareness.
- Duplicate location path within one server: error.

### Error format

```
[config] validation error: <message>
```

---

## Error Handling

All phases throw `std::runtime_error`.
Caller (WebServ::parse) catches and exits with message.
The program must not start with an invalid config.