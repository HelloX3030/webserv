#pragma once

#include "./Config.hpp"

#include <cstdint>
#include <string>
#include <vector>

/*
ConfigFrontend — compiler frontend for NGINX-style configuration files.

pipeline:
    std::string (filepath)
        │
        │ read        file → string, strip comments (# to EOL)
        v
    std::string (raw content)
        │
        │ tokenise    string → token sequence
        v
    std::vector<Token>
        │
        │ parse       tokens → structured config (recursive descent)
        v
    std::vector<ServerConfig>   (fields populated, defaults applied)
        │
        │ validate    semantic constraints the grammar cannot express
        v
    std::vector<ServerConfig>   (runtime-ready)

single public method: parse(filepath).
all pipeline machinery is private. Token and TokenType are private
nested types — no caller of parse() ever sees them.


Token and TokenType placement — 3 options considered:

1. free types in ConfigFrontend.hpp
   exposes internals to every includer.
   any Token change triggers recompilation of all includers.

2. separate internal header
   correct scoping but adds a file whose only purpose is hiding
   what the class could own itself.

3. private nested types (chosen)
   class owns its pipeline machinery.
   implementation files reach them as ConfigFrontend::Token.
   access-controlled; no leakage to public interface.


tokens_ and pos_ — shared mutable state:

these are object members, not locals passed through the call chain.
eliminates tramp data: parameters carried through every level of a
6-deep call chain not because those functions use them but because
something below does. tramp data obscures dependencies and forces
every call site to relay parameters. object members eliminate this —
private methods reach shared state through `this` implicitly.


const on navigation helpers:

peek() and at_STRING() are const: pure observation, no state change.
consume() and expect_* are not const: they advance pos_.
the read/advance asymmetry is visible in the declaration.
*/
class ConfigFrontend
{
    /* token categories.
    STRING: any char sequence not a structural char.
    structural: {, }, ;
    END: sentinel appended by tokeniser. see tokenise() postcondition. */
    enum class TokenType { STRING, LBRACE, RBRACE, SEMICOLON, END };

    struct Token
    {
        TokenType   type;
        std::string value;
        size_t      line; // source line — sole mechanism for located errors
    };

    /* shared cursor state. see header comment on tramp data elimination. */
    std::vector<Token> tokens_;
    size_t             pos_ = 0;    // in-class initialisation. born in valid state

    /* navigation — observations (const) */
    Token peek()                              const;
    bool  at_STRING(const std::string& value) const;

    /* navigation — advancement (not const) */
    Token consume();
    Token expect(TokenType type);
    Token expect_STRING();    // error: "expected directive value"
    void  expect_SEMICOLON(); // error: "expected ';'"

    /* pipeline phases */
    std::string               read    (const std::string& filepath);
    void                      tokenise(const std::string& source);
    void                      validate(const std::vector<ServerConfig>& result);


    /* grammar productions — recursive descent: 1 method per production rule.
    call hierarchy mirrors grammar nesting. grammar is specification;
    methods are direct transliteration.

    [[nodiscard]] on pure transformations.
    these fns exist solely to produce their return value.
    a caller that discards the return has accomplished nothing —
    that is a logic error detectable at compile time.
    not applied to void methods or methods whose purpose is
    side-effect (parse_server, parse_listen, etc.). */
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

    /* interpretation leaves — STRING token value → typed value */
    [[nodiscard]] size_t        parse_size     (const Token& t);
    [[nodiscard]] uint16_t      parse_port     (const std::string& s, size_t line);
    [[nodiscard]] ListenAddress parse_host_port(const Token& t);

    void validate_server  (const ServerConfig& s);
    void validate_location(const std::string& path, const Location& loc);

public:
    ConfigFrontend() = default;

    /* identity, not value semantics. copy mid-parse = bug. */
    ConfigFrontend(const ConfigFrontend&)            = delete;
    ConfigFrontend& operator=(const ConfigFrontend&) = delete;

    /* entry point. bytes → validated config.
    
    returns std::vector<ServerConfig> by value.
    caller owns the result — no ownership ambiguity, no pointer
    lifetime questions.
    
    C++17 NRVO (named return value optimisation) mandates the
    vector is constructed directly in caller's destination.
    zero copy cost. return by value is both ownership-correct
    and performance-correct. */
    [[nodiscard]] std::vector<ServerConfig> parse(const std::string& filepath);
};