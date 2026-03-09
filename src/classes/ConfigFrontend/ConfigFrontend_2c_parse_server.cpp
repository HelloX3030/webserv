/* server-level directive parsers.

dispatch contract (set by parse_server in 2b):
    directive name already consumed.
    pos_ is at the first value token on entry.
    each function owns its values and the terminating semicolon. */


/* grammar: listen_dir = "listen", host_port, ";" ;

bare port ("8080") or host:port ("127.0.0.1:8080").
multiple listen directives permitted — appends to s.listen. */
void Frontend::parse_listen(ServerConfig& s)
{
    Token t = expect_STRING();
    s.listen.push_back(parse_host_port(t));
    expect_SEMICOLON();
}

/* grammar: server_name_dir = "server_name", name, { name }, ";" ;

at least 1 name required. multiple names permitted — virtual hosting. */
void Frontend::parse_server_name(ServerConfig& s)
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

maps HTTP status code to URI path.
valid range [100, 599] enforced here and confirmed in validator. */
void Frontend::parse_error_page(ServerConfig& s)
{
    Token code_tok = expect_STRING();
    Token path_tok = expect_STRING();

    int code = 0;
    try { code = std::stoi(code_tok.value); }
    catch (...)
    {
        throw std::runtime_error(
            "[config] line " + std::to_string(code_tok.line) + "\n"
            "  error_page: invalid status code.\n"
            "  provided: '" + code_tok.value + "'");
    }

    if (code < 100 || code > 599)
        throw std::runtime_error(
            "[config] line " + std::to_string(code_tok.line) + "\n"
            "  error_page: code out of range.\n"
            "  provided:      " + code_tok.value + "\n"
            "  defined range: [100, 599]");

    s.error_pages[static_cast<uint16_t>(code)] = path_tok.value;
    expect_SEMICOLON();
}

/* grammar: body_size_dir = "client_max_body_size", size, ";" ;

size suffix (k/K, m/M, g/G) interpreted by parse_size. */
void Frontend::parse_body_size(ServerConfig& s)
{
    Token t = expect_STRING();
    s.client_max_body_size = parse_size(t);
    expect_SEMICOLON();
}