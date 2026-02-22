#pragma once

#include "./Config.hpp"

#include <cstdint>
#include <string>
#include <vector>

class ConfigParser
{
    enum class TokenType { STRING, LBRACE, RBRACE, SEMICOLON, END };

    struct Token
    {
        TokenType   type;
        std::string value;
        size_t      line; // carried for error messages
    };

    // shared cursor state across all private methods via `this`
    std::vector<Token> tokens_;
    size_t             pos_;

    // observations
    Token peek()                              const;
    bool  at_STRING(const std::string& value) const;

    Token consume();
    Token expect(TokenType type);
    Token expect_STRING();    // error: "expected directive value"
    void  expect_SEMICOLON(); // error: "expected ';'"

    // pipeline phases
    std::string               read    (const std::string& filepath);
    void                      tokenise(const std::string& source);
    void                      validate(const std::vector<ServerConfig>& result);

    /* 1 method per grammar production rule */
    [[nodiscard]] std::vector<ServerConfig> parse_config();
    [[nodiscard]] ServerConfig              parse_server_block();
    [[nodiscard]] Location                  parse_location_block();

    void parse_server  (ServerConfig& s);
    void parse_location(Location& loc);

    void parse_listen     (ServerConfig& s);
    void parse_server_name(ServerConfig& s);
    void parse_error_page (ServerConfig& s);
    void parse_body_size  (ServerConfig& s); // assigns to size_t
    void parse_body_size  (Location& loc);   // wraps in optional

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

    void validate_server  (const ServerConfig& s);
    void validate_location(const std::string& path, const Location& loc);

public:
    ConfigParser() = default;

    /* this obj has identity, no val. semantics. cpy mid-parse = bug */
    ConfigParser(const ConfigParser&)            = delete;
    ConfigParser& operator=(const ConfigParser&) = delete;

    [[nodiscard]] std::vector<ServerConfig> parse(const std::string& filepath);
};