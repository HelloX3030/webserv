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
std::vector<ServerConfig>  (fields populated, defaults applied)
    |
    | Phase 4: Validation
    v
std::vector<ServerConfig>  (semantically verified)
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
    STRING,
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

`line` is carried on every token. It is the only mechanism
for producing precise error messages in later phases.

### Rules

- `{`  → LBRACE
- `}`  → RBRACE
- `;`  → SEMICOLON
- anything else → STRING

### Why no NUMBER token type — decision and reasoning

The token type for a digit-leading sequence such as `8080`
is STRING, not a separate NUMBER type.

The lexer's contract is to recognise *shape* — the structural
boundaries between tokens. A token is any unbroken sequence
of non-whitespace, non-brace, non-semicolon characters.
`8080` and `localhost` satisfy this definition identically.
The lexer cannot distinguish them without knowing *meaning*,
and meaning-awareness is the parser's job, not the lexer's.

Concretely: `8080` appearing after `listen` means port.
`8080` appearing after `server_name` would be a hostname.
`8080` does not carry its own meaning — the grammar position
determines it. Therefore the type distinction must live in
the parser, not the lexer.

A NUMBER type would require the lexer to classify digit-leading
strings as semantically special. This violates the single-
responsibility boundary between phases. The lexer would be
doing partial meaning-assignment — incorrect.

Consequence for the parser: every directive helper that expects
a numeric value (parse_port, parse_size, parse_error_page)
consumes a STRING token and interprets its value string via
std::stoi or equivalent. If the string does not convert,
the parser throws with the token's line number.

```cpp
// example: parse_listen_dir
void ConfigParser::parse_listen_dir(ServerConfig& s) {
    Token t = consume();           // type: STRING, value: "8080" or "127.0.0.1:8080"
    expect_semicolon();
    s.listen.push_back(parse_host_port(t));
}

ListenAddress ConfigParser::parse_host_port(const Token& t) {
    // try to find ':' — if present, split host and port
    // then call parse_port() on the port substring
}

uint16_t ConfigParser::parse_port(const std::string& s, size_t line) {
    try {
        int val = std::stoi(s);
        if (val < 1 || val > 65535)
            throw std::runtime_error(/* ... */);
        return static_cast<uint16_t>(val);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("[config] line " + std::to_string(line)
            + ": expected port number, got '" + s + "'");
    }
}
```

This is more logic in the parser — intentionally. The parser is
where grammar and meaning converge. The lexer is kept minimal
and mechanical.

### Lexer does not

- validate values (port range, path existence, etc.)
- classify tokens by semantic meaning
- interpret size suffixes (`10m`, `1k`)
- distinguish directive names from values or paths

---

## Phase 3 — Parser

### Structure

Recursive descent. 1 function per grammar production rule.
The grammar is the specification; the code is a direct
translation of it.

### Token and TokenType — private nested types

`Token` and `TokenType` appear conceptually in Phase 2 as the output
type of the lexer. Their definition belongs inside `ConfigParser` as
private nested types.

The question is: who needs to see `Token`?

The tokeniser (`ConfigParser_tokenise.cpp`) produces tokens.
The parser (`ConfigParser_parse.cpp`) consumes them.
No caller of `parse()` ever sees a token. No file outside the
ConfigParser implementation units has any use for these types.

3 options were considered:

1. Free types in `ConfigParser.hpp` — makes implementation details
   visible to any file that includes the header. A file that calls
   `parse()` and never touches a token still sees Token in scope.
   Unnecessary exposure.

2. Separate internal header `ConfigParser_internal.hpp` — included
   only by the 2 implementation files. Correctly restricts scope,
   but adds a file whose sole purpose is to hide information that the
   class itself could simply own.

3. Private nested types inside `ConfigParser` — the class owns its
   pipeline machinery. `Token` and `TokenType` are syntactically
   present in the header but access-controlled: external code cannot
   name them. The implementation files reach them as
   `ConfigParser::Token` and `ConfigParser::TokenType`.

Option 3 is the correct choice. It expresses the ownership relation
directly: `Token` is an artefact of `ConfigParser`'s internal
pipeline, not a type belonging to the config subsystem's public
vocabulary. No separate file, no leaked scope.

The class declaration therefore opens with:
```cpp
class ConfigParser {
    enum class TokenType { STRING, LBRACE, RBRACE, SEMICOLON, END };

    struct Token {
        TokenType   type;
        std::string value;
        size_t      line;
    };

    // remainder of private state and methods...
```

`TokenType::END` is the sentinel appended by the tokeniser as its
final element. It allows `peek()` to return a well-defined token at
stream exhaustion without an unchecked index — the outer parse loop
terminates on `END` rather than on `pos >= tokens.size()`. The
tokeniser's postcondition: the last element of `tokens` is always
`Token{TokenType::END, "", last_line}`.


```cpp
class ConfigParser {
    std::vector<Token> tokens;
    size_t             pos;

    Token   peek();
    Token   consume();
    Token   expect(TokenType type);    // consume or throw
    Token   expect_STRING();             // consume STRING or throw
    void    expect_semicolon();        // consume SEMICOLON or throw
    bool    at_STRING(const std::string& value);

    std::vector<ServerConfig>  parse_config();
    ServerConfig    parse_server_block();
    void    parse_server_dir(ServerConfig& s);
    void    parse_listen_dir(ServerConfig& s);
    void    parse_server_name_dir(ServerConfig& s);
    void    parse_error_page_dir(ServerConfig& s);
    Location    parse_location_block();
    void    parse_location_dir(Location& loc);
    void    parse_root_dir(Location& loc);
    void    parse_index_dir(Location& loc);
    void    parse_methods_dir(Location& loc);
    void    parse_autoindex_dir(Location& loc);
    void    parse_cgi_ext_dir(Location& loc);
    void    parse_cgi_path_dir(Location& loc);
    void    parse_upload_enable_dir(Location& loc);
    void    parse_upload_store_dir(Location& loc);
    void    parse_return_dir(Location& loc);

    size_t  parse_size(const Token& t);
    uint16_t    parse_port(const std::string& s, size_t line);
    ListenAddress   parse_host_port(const Token& t);

public:
    std::vector<ServerConfig>   parse(const std::string& filepath);
};
```

parse_size is the single leaf for body size interpretation. It returns a size_t. 
The 2 call sites differ in assignment type: 
. parse_server_dir assigns directly to ServerConfig::client_max_body_size (a size_t); 
. parse_location_dir wraps the result in std::optional. 
A shared parse_body_size_dir(size_t&) cannot serve both 
without forcing the caller to hold an intermediate. 
The leaf is general; the assignment is local. No shared intermediate method is needed.

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
[config] line 17: expected port number, got 'abc'
```

### Navigation helpers

expect(TokenType) is the base: consume the next token, 
throw if type does not match. It reports a type name. 

expect_STRING() and expect_SEMICOLON() are specialisations 
that carry semantic context: at a parse site that calls expect_STRING(), 
we know we are at a position in the grammar where a directive value 
or identifier is required — the error message can say "expected directive value" 
rather than "expected STRING". expect_SEMICOLON() always says "expected ';'". 

This is not redundancy. It is the difference between reporting 
what the grammar received and what the grammar position demands. 
These specialisations are the mechanism for error messages 
that orient the operator in the config file, not merely in the token stream.

---

## Phase 4 — Validation

Runs after the full std::vector<ServerConfig> is built.
Checks semantic constraints that the grammar cannot express.

### Mandatory fields

| Level    | Field    | Rule                                    |
|----------|----------|-----------------------------------------|
| Server   | listen   | at least one ListenAddress required     |
| Server   | locations| at least one location block required    |
| Location | root     | required                                |

### Value range checks

| Field                  | Constraint     |
|------------------------|----------------|
| ListenAddress::port    | [1, 65535]     |
| error_pages key        | [100, 599]     |
| client_max_body_size   | > 0            |

### Semantic coupling

- cgi_extension and cgi_path: either both set or both absent.
  One without the other is an error.
- return_code and return_path: either both set or both absent.

### Duplicate checks

- Duplicate listen address+port pair across servers:
  valid (spec allows multiple servers on same port for
  virtual hosting). note but do not error.
- Duplicate location path within one server: error.

### Error format

```
[config] validation error: <message>
```

---

## Error Handling

All phases throw `std::runtime_error`.
Caller (main / WebServ::init) catches and exits with message.
The program must not start with an invalid config.