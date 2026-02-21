#include "../../../include/classes/ConfigParser.hpp"

#include <stdexcept>
#include <string>

/* listen_dir: grammar: "listen", host_port, ";" ;
multiple listen directives valid — each appends to listen. */
void ConfigParser::parse_listen(ServerConfig& s)
{
    Token t = expect_STRING();
    s.listen.push_back(parse_host_port(t));
    expect_SEMICOLON();
}

/* server_name_dir: grammar: "server_name", name, { name }, ";" ;
grammar requires at least 1 name. */
void ConfigParser::parse_server_name(ServerConfig& s)
{
    if (peek().type != TokenType::STRING)
        throw std::runtime_error(
            "[config] line " + std::to_string(peek().line) +
            ": server_name requires at least 1 name");

    while (peek().type == TokenType::STRING)
        s.server_names.push_back(consume().value);

    expect_SEMICOLON();
}

/* error_page_dir: grammar: "error_page", status_code, path, ";" ;
status_code range [100, 599] enforced here; validator confirms. */
void ConfigParser::parse_error_page(ServerConfig& s)
{
    Token code_tok = expect_STRING();
    Token path_tok = expect_STRING();

    int code;
    try { code = std::stoi(code_tok.value); }
    catch (...)
    {
        throw std::runtime_error(
            "[config] line " + std::to_string(code_tok.line) +
            ": invalid status code '" + code_tok.value + "'");
    }
    if (code < 100 || code > 599)
        throw std::runtime_error(
            "[config] line " + std::to_string(code_tok.line) +
            ": status code out of range [100, 599]: " + code_tok.value);

    s.error_pages[static_cast<uint16_t>(code)] = path_tok.value;
    expect_SEMICOLON();
}

/* body_size_dir (server): grammar: "client_max_body_size", size, ";" ;
assigns directly to ServerConfig::client_max_body_size (size_t).
overload resolved at the call site in parse_server by argument type. */
void ConfigParser::parse_body_size(ServerConfig& s)
{
    Token t = expect_STRING();
    s.client_max_body_size = parse_size(t);
    expect_SEMICOLON();
}