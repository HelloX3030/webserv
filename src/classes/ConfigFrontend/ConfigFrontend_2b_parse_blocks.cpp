/* grammar productions: config, server_block, location_block.

recursive descent: each production becomes a fn.
call hierarchy mirrors grammar nesting:

    parse_config
      └─ parse_server_block
           ├─ parse_server          (flat directive dispatch)
           │    └─ parse_listen / parse_server_name / ...
           └─ parse_location_block  (nested block)
                └─ parse_location   (flat directive dispatch)
                     └─ parse_root / parse_methods / ...

"recursive" here denotes structural mirroring, not self-calls.
self-calls would appear only if the grammar were self-referential
(e.g. nested location blocks). this grammar has no such production. */


/* top-level production.
grammar: config = server_block, { server_block } ;

at least 1 server block required — empty config rejected.
loop terminates on END sentinel. */
std::vector<ServerConfig> Frontend::parse_config()
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
        throw std::runtime_error(
            "[config] line " + std::to_string(peek().line) +
            ": no server block found");

    return result;
}

/* grammar: server_block = "server", "{", { server_dir | location_block }, "}" ;

"server" already consumed by parse_config.

client_max_body_size defaults to DEFAULT_CLIENT_MAX_BODY_SIZE.
   directive presence overrides; absence means default persists.
*/
ServerConfig Frontend::parse_server_block()
{
    ServerConfig s;
    s.client_max_body_size = DEFAULT_CLIENT_MAX_BODY_SIZE;

    expect(TokenType::LBRACE);

    while (peek().type != TokenType::RBRACE)
    {
        if (peek().type == TokenType::END)
            throw std::runtime_error(
                "[config] line " + std::to_string(peek().line) +
                ": unterminated server block");

        if (at_STRING("location"))
        {
            consume(); // "location" keyword spent as dispatch decision — path next
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

dispatch contract: consume directive name, delegate to specific parser.
specific parser enters with pos_ at first value token and owns
all values and the terminating semicolon.

dispatch: if-chain. alternatives (std::map<std::string, std::function>,
switch on hash) add allocation, indirection, or fragility under additions.
if-chain is readable, debuggable, correct. performance irrelevant:
parser runs once at startup over ~10 directives. */
void Frontend::parse_server(ServerConfig& s)
{
    Token name = expect_STRING();

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
    autoindex       = false
    upload_enable   = false

allowed_methods default rationale: all methods permitted unless
explicitly restricted. default {GET} (silence = GET only) was
rejected: too restrictive for an evaluation server where the evaluator
expects all methods unless explicitly limited. */
Location Frontend::parse_location_block()
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
same contract as parse_server: consume name, delegate, specific parser
owns values and semicolon. */
void Frontend::parse_location(Location& loc)
{
    Token name = expect_STRING();

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