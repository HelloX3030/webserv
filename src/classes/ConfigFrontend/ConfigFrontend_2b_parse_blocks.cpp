#include "../../../include/classes/ConfigFrontend.hpp"

#include <stdexcept>
#include <string>

/* top-level production.
grammar: config = server_block, { server_block } ;
at least 1 server block required — an empty config is rejected. */
std::vector<ServerConfig> ConfigFrontend::parse_config()
{
    std::vector<ServerConfig> result;

    while (peek().type != TokenType::END)
    {
        Token t = expect_STRING();
        if (t.value != "server")
            throw std::runtime_error(
                "[config] line " + std::to_string(t.line) +
                ": expected 'server', got '" + t.value + "'");
        result.push_back(parse_server_block());
    }

    if (result.empty())
        throw std::runtime_error("[config]: no server block found");

    return result;
}

/* grammar: server_block = "server", "{", { server_dir }, "}" ;
"server" already consumed by parse_config.
struct initialised with defaults before the directive loop —
any directive present in config overrides. */
ServerConfig ConfigFrontend::parse_server_block()
{
    ServerConfig s;
    s.client_max_body_size = 1048576; // 1m default

    expect(TokenType::LBRACE);

    while (peek().type != TokenType::RBRACE)
    {
        if (peek().type == TokenType::END)
            throw std::runtime_error(
                "[config] line " + std::to_string(peek().line) +
                ": unterminated server block");

        if (at_STRING("location"))
        {
            consume(); // "location"
            Token path_tok = expect_STRING();
            
            if (s.locations.count(path_tok.value))
                throw std::runtime_error(
                    "[config] line " + std::to_string(path_tok.line) +
                    ": duplicate location path '" + path_tok.value + "'");
            s.locations[path_tok.value] = parse_location_block();
        }
        else
        {
            parse_server(s);
        }
    }

    expect(TokenType::RBRACE);
    return s;
}

/* dispatch a server-level directive.
consumes the directive name — spent as the dispatch key.
specific parser enters with pos_ at the 1st value token.
the specific parser owns values and the terminating semicolon.
violating this contract (specific parser re-consuming its own name)
produces off-by-1 token errors throughout. */
void ConfigFrontend::parse_server(ServerConfig& s)
{
    Token name = consume();

    if (name.type != TokenType::STRING)
        throw std::runtime_error(
            "[config] line " + std::to_string(name.line) +
            ": expected directive name, got '" + name.value + "'");

    if (name.value == "listen")               { parse_listen(s);      return; }
    if (name.value == "server_name")          { parse_server_name(s); return; }
    if (name.value == "client_max_body_size") { parse_body_size(s);   return; }
    if (name.value == "error_page")           { parse_error_page(s);  return; }

    throw std::runtime_error(
        "[config] line " + std::to_string(name.line) +
        ": unknown server directive '" + name.value + "'");
}

/* grammar: location_block = "location", path, "{", { location_dir }, "}" ;
"location" and path already consumed by parse_server_block.
defaults applied before directive loop. */
Location ConfigFrontend::parse_location_block()
{
    Location loc;
    loc.allowed_methods = {HttpMethod::GET, HttpMethod::POST, HttpMethod::DELETE};
    loc.autoindex       = false;
    loc.upload_enable   = false;

    expect(TokenType::LBRACE);

    while (peek().type != TokenType::RBRACE)
    {
        if (peek().type == TokenType::END)
            throw std::runtime_error(
                "[config] line " + std::to_string(peek().line) +
                ": unterminated location block");

        parse_location(loc);
    }

    expect(TokenType::RBRACE);
    return loc;
}

/* same contract as parse_server: 
consumes name, specific parser owns values. */
void ConfigFrontend::parse_location(Location& loc)
{
    Token name = consume();

    if (name.type != TokenType::STRING)
        throw std::runtime_error(
            "[config] line " + std::to_string(name.line) +
            ": expected directive name, got '" + name.value + "'");

    if (name.value == "root")                 { parse_root(loc);          return; }
    if (name.value == "index")                { parse_index(loc);         return; }
    if (name.value == "allowed_methods")      { parse_methods(loc);       return; }
    if (name.value == "autoindex")            { parse_autoindex(loc);     return; }
    if (name.value == "cgi_extension")        { parse_cgi_ext(loc);       return; }
    if (name.value == "cgi_path")             { parse_cgi_path(loc);      return; }
    if (name.value == "client_max_body_size") { parse_body_size(loc);     return; }
    if (name.value == "upload_enable")        { parse_upload_enable(loc); return; }
    if (name.value == "upload_store")         { parse_upload_store(loc);  return; }
    if (name.value == "return")               { parse_return(loc);        return; }

    throw std::runtime_error(
        "[config] line " + std::to_string(name.line) +
        ": unknown location directive '" + name.value + "'");
}