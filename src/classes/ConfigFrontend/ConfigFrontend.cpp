#include "ConfigFrontend.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

/*
TokenType — structural character classes of the config lexer.

STRING:    any byte sequence not a delimiter or whitespace.
LBRACE:    {
RBRACE:    }
SEMICOLON: ;
END:       see 1_tokenise
*/
enum class TokenType { STRING, LBRACE, RBRACE, SEMICOLON, END };

struct Token
{
    TokenType   type;
    std::string value;
    size_t      line; // source line at emission — sole carrier of
                      // location information for parse-time errors.
};

/*
parse scope — the binding site for state shared across the recursive
descent call tree. exists because the call tree requires it, not
because the domain requires it. lifetime: 1 call to parse().
*/
struct Frontend
{
    /* token stream & read head:
    2 shared cursors: the complete mutable parse state,
    no tramp data */
    std::vector<Token> tokens_;
    size_t             pos_ = 0;

    /* source acquisition */
    std::string read    (const std::string& filepath);
    void        tokenise(const std::string& source);

    /* navigation — observations */
    Token peek()                              const;
    bool  at_STRING(const std::string& value) const;

    /* navigation — advancement */
    Token consume();
    Token expect       (TokenType type);
    Token expect_STRING();
    void  expect_SEMICOLON();

    /* grammar productions. */
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

    /* interpretation — STRING token value → typed value */
    [[nodiscard]] size_t        parse_size     (const Token& t);
    [[nodiscard]] uint16_t      parse_port     (const std::string& s, size_t line);
    [[nodiscard]] ListenAddress parse_host_port(const Token& t);

    void validate         (const std::vector<ServerConfig>& servers);
    void validate_server  (const ServerConfig& s);
    void validate_location(const std::string& path, const Location& loc);
};

} // anonymous namespace

namespace ConfigFrontend {

/*
parse() sequences the pipeline. Frontend owns state & stage implementations.
failure at any stage throws std::runtime_error with a located message.
caller catches once.
*/
std::vector<ServerConfig> parse(const std::string& filepath)
{
    Frontend    f;
    std::string source = f.read(filepath);
    f.tokenise(source);
    auto result = f.parse_config();
    f.validate(result);
    return result;
}

} // namespace ConfigFrontend

#include "ConfigFrontend_0_read.cpp"
#include "ConfigFrontend_1_tokenise.cpp"
#include "ConfigFrontend_2a_parse_navigate.cpp"
#include "ConfigFrontend_2b_parse_blocks.cpp"
#include "ConfigFrontend_2c_parse_server.cpp"
#include "ConfigFrontend_2d_parse_location.cpp"
#include "ConfigFrontend_2e_interpret.cpp"
#include "ConfigFrontend_3_validate.cpp"
