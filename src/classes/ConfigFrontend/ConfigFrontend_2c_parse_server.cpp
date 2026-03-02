#include "../../../include/classes/ConfigFrontend.hpp"

#include <stdexcept>
#include <string>

/* server-level directive parsers.

each function implements one grammar production from server_dir.
directive name already consumed by parse_server (dispatch contract).
function enters with pos_ at first value token.
function owns values and terminating semicolon. */


/* grammar: listen_dir = "listen", host_port, ";" ;

host_port may be:
    bare port:     "8080"         → host defaults to "0.0.0.0"
    host:port:     "127.0.0.1:8080"

multiple listen directives permitted — server may bind multiple addresses.
each call appends to s.listen vector. */
void ConfigFrontend::parse_listen(ServerConfig& s)
{
    Token t = expect_STRING();
    s.listen.push_back(parse_host_port(t));
    expect_SEMICOLON();
}

/* grammar: server_name_dir = "server_name", name, { name }, ";" ;

at least 1 name required. multiple names permitted — virtual hosting.
names accumulate across multiple server_name directives if present
(though typically only one directive is used). */
void ConfigFrontend::parse_server_name(ServerConfig& s)
{
    if (peek().type != TokenType::STRING)
        throw std::runtime_error(
            "[config] line " + std::to_string(peek().line) +
            ": server_name requires at least 1 name");

    while (peek().type == TokenType::STRING)
        s.server_names.push_back(consume().value);

    expect_SEMICOLON();
}

/* grammar: error_page_dir = "error_page", status_code, path, ";" ;

status_code: 3-digit integer. valid range [100, 599] checked here
(parse-time rejection) and confirmed in validator (belt-and-suspenders).

maps status code to URI path. runtime serves this path on error. */
void ConfigFrontend::parse_error_page(ServerConfig& s)
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
            ": error_page code out of range [100, 599]: " + code_tok.value);

    s.error_pages[static_cast<uint16_t>(code)] = path_tok.value;
    expect_SEMICOLON();
}

/* grammar: body_size_dir = "client_max_body_size", size, ";" ;

server-level: assigns directly to s.client_max_body_size.
this is the default inherited by locations with nullopt.

size suffix (k, m, g) interpreted by parse_size. */
void ConfigFrontend::parse_body_size(ServerConfig& s)
{
    Token t = expect_STRING();
    s.client_max_body_size = parse_size(t);
    expect_SEMICOLON();
}