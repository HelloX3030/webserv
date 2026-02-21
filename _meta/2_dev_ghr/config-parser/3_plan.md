# config parser — implementation plan

---

## pipeline

linear, 4 phases. single input/output type per phase.
failure at any phase throws with a precise error message.

```
std::string (filepath)
    |
    | phase 1: file read
    v
std::string (raw content)
    |
    | phase 2: lexer
    v
std::vector<Token>
    |
    | phase 3: parser
    v
std::vector<ServerConfig>   (fields populated, defaults applied)
    |
    | phase 4: validation
    v
std::vector<ServerConfig>   (semantically verified)
```

---

## phase 1 — file read

read entire file into std::string.
strip comments: from # to end of line, replace with whitespace.
whitespace replacement (not deletion) keeps line numbers accurate.

---

## phase 2 — lexer

```
std::string → void   (populates tokens_)
```

### token and tokentype

Token and TokenType are private nested types inside ConfigParser.
no caller of parse() ever sees a token. 3 options considered:

1. free types in ConfigParser.hpp — exposes internals to every
   includer; any change triggers recompilation of all includers.
2. separate internal header — correct scoping but adds a file whose
   only purpose is hiding what the class could own itself.
3. private nested types — chosen. class owns its pipeline machinery.
   access-controlled; implementation files reach them as
   ConfigParser::Token, ConfigParser::TokenType.

```cpp
enum class TokenType { STRING, LBRACE, RBRACE, SEMICOLON, END };

struct Token {
    TokenType   type;
    std::string value;
    size_t      line;
};
```

`line` is carried on every token — the only mechanism for precise
error messages in later phases.

`END` is a sentinel appended by the tokeniser as its final element.
allows peek() to return a valid token at stream exhaustion without
an unchecked index. the outer parse loop terminates on END rather
than testing pos_ against tokens_.size().
tokeniser postcondition: tokens_.back() is always Token{END, "", last_line}.

### lexer rules

- `{`            → LBRACE
- `}`            → RBRACE
- `;`            → SEMICOLON
- anything else  → STRING

### why no NUMBER type

`8080` after `listen` means port. `8080` after `server_name` would
be a hostname. the token does not carry its own meaning — grammar
position determines it. a NUMBER type would require the lexer to
classify digit-leading strings as semantically special, violating
the single-responsibility boundary: the lexer would be doing partial
meaning-assignment, which belongs to the parser.

consequence: every directive parser that expects a numeric value
consumes a STRING token and interprets its value via std::stoi.
if the string does not convert, the parser throws with the line number.

### lexer does not

- validate values (port range, path existence, etc.)
- classify tokens by semantic meaning
- interpret size suffixes (`10m`, `1k`)
- distinguish directive names from values or paths

---

## phase 3 — parser

### structure

recursive descent. 1 method per grammar production rule.
grammar is the specification; methods are a direct transliteration.

```cpp
class ConfigParser
{
    enum class TokenType { STRING, LBRACE, RBRACE, SEMICOLON, END };

    struct Token {
        TokenType   type;
        std::string value;
        size_t      line;
    };

    std::vector<Token> tokens_;
    size_t             pos_;

    // observations — const (no state change)
    Token peek()                              const;
    bool  at_STRING(const std::string& value) const;

    // advancement — not const
    Token consume();
    Token expect(TokenType type);
    Token expect_STRING();
    void  expect_SEMICOLON();

    // pipeline phases
    std::string               read    (const std::string& filepath);
    void                      tokenise(const std::string& source);
    void                      validate(const std::vector<ServerConfig>& result);

    // grammar methods — 1 per production rule
    [[nodiscard]] std::vector<ServerConfig> parse_config();
    [[nodiscard]] ServerConfig              parse_server_block();
    [[nodiscard]] Location                  parse_location_block();

    void parse_server  (ServerConfig& s);
    void parse_location(Location& loc);

    void parse_listen     (ServerConfig& s);
    void parse_server_name(ServerConfig& s);
    void parse_error_page (ServerConfig& s);
    void parse_body_size  (ServerConfig& s);
    void parse_body_size  (Location& loc);

    void parse_root         (Location& loc);
    void parse_index        (Location& loc);
    void parse_methods      (Location& loc);
    void parse_autoindex    (Location& loc);
    void parse_cgi_ext      (Location& loc);
    void parse_cgi_path     (Location& loc);
    void parse_upload_enable(Location& loc);
    void parse_upload_store (Location& loc);
    void parse_return       (Location& loc);

    // interpretation leaves — STRING token value → typed value
    [[nodiscard]] size_t        parse_size     (const Token& t);
    [[nodiscard]] uint16_t      parse_port     (const std::string& s, size_t line);
    [[nodiscard]] ListenAddress parse_host_port(const Token& t);

public:
    ConfigParser() = default;
    ConfigParser(const ConfigParser&)            = delete;
    ConfigParser& operator=(const ConfigParser&) = delete;

    [[nodiscard]] std::vector<ServerConfig> parse(const std::string& filepath);
};
```

### navigation helpers

tokens_ and pos_ are object members, not locals passed through the
call chain, to eliminate tramp data — parameters carried through
every level of a deep call chain not because those functions use
them but because something below does. object members are visible
to all private methods via this, with no parameter overhead.

peek() and at_STRING() are const: pure observations, no state change.
consume() and the expect_* family are not const: they advance pos_.
the read/advance asymmetry is visible in the declaration.

expect_STRING() and expect_SEMICOLON() are not aliases for
expect(TokenType::STRING/SEMICOLON). they carry semantic context:
at a call site where expect_STRING() is used, the grammar demands a
directive value or identifier — the error says "expected directive
value", not "expected STRING". expect_SEMICOLON() always produces
"expected ';'". difference: reporting what the token stream received
vs what the grammar position requires.

### directive consumption contract

parse_server and parse_location consume the directive name token
before dispatching to the specific parser. the specific parser enters
with pos_ at the first value token. it consumes values and the
terminating semicolon, then returns.

rationale: the name token is spent as the dispatch decision.
consuming it again inside the specific parser would be redundant
and would create a hidden coupling: every specific parser would need
to know to skip its own name. that is a trap for every future
addition. the contract is: dispatch owns the name; specific parser
owns the values.

violating this contract produces off-by-one token errors.

### parse_body_size — overloads, single leaf

parse_size(const Token&) → size_t is the single interpretation leaf.
2 call sites assign differently:

- server level: assigns directly to ServerConfig::client_max_body_size (size_t)
- location level: wraps in std::optional<size_t>

a shared method with size_t& out cannot serve both. the leaf is
general; assignment is local to each caller. parse_body_size is
overloaded on parameter type (ServerConfig& / Location&). both
call parse_size internally. overload resolution is by argument type
at the call site — no naming distinction needed.

### defaults applied here

struct is initialised with defaults before any directives are parsed.
see 2_data-model.md defaults table.

### error format

```
[config] line <N>: <message>
```

examples:

```
[config] line 12: expected ';'
[config] line 8:  unknown directive 'listen2'
[config] line 5:  expected '{' after 'server'
[config] line 17: expected port number, got 'abc'
```

### parse_port example

```cpp
uint16_t ConfigParser::parse_port(const std::string& s, size_t line)
{
    try {
        int val = std::stoi(s);
        if (val < 1 || val > 65535)
            throw std::runtime_error(
                "[config] line " + std::to_string(line)
                + ": port out of range [1, 65535]: '" + s + "'");
        return static_cast<uint16_t>(val);
    }
    catch (const std::invalid_argument&) {
        throw std::runtime_error(
            "[config] line " + std::to_string(line)
            + ": expected port number, got '" + s + "'");
    }
}
```

---

## phase 4 — validation

runs after the full std::vector<ServerConfig> is built.
checks semantic constraints the grammar cannot express.

### mandatory fields

| level    | field     | rule                                |
|----------|-----------|-------------------------------------|
| server   | listen    | at least 1 ListenAddress required   |
| server   | locations | at least 1 location block required  |
| location | root      | required                            |

### value range checks

| field                | constraint  |
|----------------------|-------------|
| ListenAddress::port  | [1, 65535]  |
| error_pages key      | [100, 599]  |
| client_max_body_size | > 0         |

### semantic coupling

- cgi_ext and cgi_path: either both set or both absent.
- return_code and return_path: either both set or both absent.
- upload_enable true requires upload_store non-empty.

### duplicate checks

- duplicate listen address:port across servers: valid (virtual
  hosting). note but do not error.
- duplicate location path within 1 server: error.

### error format

```
[config] validation error: <message>
```

---

## error handling

all phases throw std::runtime_error.
caller (main / WebServ::init) catches and exits with the message.
the program must not start with an invalid config.