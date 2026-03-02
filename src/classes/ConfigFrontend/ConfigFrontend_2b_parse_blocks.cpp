#include "../../../include/classes/ConfigFrontend.hpp"

#include <stdexcept>
#include <string>

/* grammar productions: config, server_block, location_block.

recursive descent: each grammar production becomes a function.
the call hierarchy mirrors the grammar nesting:

    grammar:
        config → server_block → { server_dir | location_block }
                                     location_block → { location_dir }

    call tree:
        parse_config
          └─ parse_server_block
               ├─ parse_server          (flat directives)
               │    └─ parse_listen / parse_server_name / ...
               └─ parse_location_block  (nested block)
                    └─ parse_location
                         └─ parse_root / parse_methods / ...

"descent": parser begins at top-level production (parse_config)
and descends toward terminal tokens.

"recursive": the structural mirroring, not self-calls. self-calls
would appear only if grammar were self-referential (e.g. nested
location blocks). this grammar has no such nesting. */


/* top-level production.
grammar: config = server_block, { server_block } ;

at least 1 server block required — empty config rejected.
loop terminates on END sentinel. */
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

defaults applied before directive loop:
    client_max_body_size = 1048576 (1M)
    
any directive present in config overrides. absence of directive
means default persists. see Config.hpp for full defaults table. */
ServerConfig ConfigFrontend::parse_server_block()
{
    ServerConfig s;
    s.client_max_body_size = 1048576;

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

/* directive dispatch for server-level directives.

directive consumption contract:
    this function consumes the directive name token.
    the name is spent as the dispatch decision.
    specific parser (parse_listen, etc.) enters with pos_ at 1st value token.
    specific parser owns values and terminating semicolon.

dispatch mechanism: if-chain.

alternatives considered:
    std::map<std::string, std::function> — adds indirection (std::function
    involves allocation and virtual call) and noise (map construction).
    
    switch on hash — fast but requires collision-free compile-time hash.
    fragile under directive additions.

if-chain chosen: readable, debuggable (each branch visible), correct.
performance irrelevant — parser runs once at startup with ~10 directives. */
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

defaults applied before directive loop:
    allowed_methods = {GET, POST, DELETE}   (all methods permitted)
    autoindex       = false                  (no directory listing)
    upload_enable   = false                  (uploads disabled)

allowed_methods default rationale: all methods permitted unless
explicitly restricted. alternative considered: default {GET} (silence
means GET only). rejected: too restrictive for evaluation server where
evaluator expects all methods unless explicitly limited. */
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

/* directive dispatch for location-level directives.
same contract as parse_server: consumes name, specific parser owns values. */
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