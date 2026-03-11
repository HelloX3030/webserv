/* location-level directive parsers.

each function implements 1 grammar production from location_dir.
same contract as server-level: name consumed, function owns values.

memory-mechanics:
This fragment has no memory safety exposure of its own.
Its correctness is contingent on the END sentinel invariant
established by fragment 1. the tokeniser's final push unconditionally
appends Token{TokenType::END, "", line}
*/

/* grammar: root_dir = "root", path, ";" ;

filesystem path. runtime resolves request URIs against this root.
mandatory field — validator rejects location without root. */
void Frontend::parse_root(Location& loc)
{
    loc.root = expect_STRING().value;
    expect_SEMICOLON();
}

/* grammar: index_dir = "index", filename, { filename }, ";" ;

at least 1 filename required. when URI maps to directory, server
tries each index file in order until one exists. */
void Frontend::parse_index(Location& loc)
{
    if (peek().type != TokenType::STRING)
        throw std::runtime_error(
            "[config] line " + std::to_string(peek().line) +
            ": index requires at least 1 filename");
    while (peek().type == TokenType::STRING)
        loc.index_files.push_back(consume().value);
    expect_SEMICOLON();
}

/* grammar: methods_dir = "allowed_methods", method, { method }, ";" ;

clears default {GET, POST, DELETE}: directive is an override, not additive.
std::set deduplicates: "allowed_methods GET GET;" → {GET}. */
void Frontend::parse_methods(Location& loc)
{
    loc.allowed_methods.clear();
    while (peek().type == TokenType::STRING)
    {
        Token t = consume();
        if      (t.value == "GET")    loc.allowed_methods.insert(HttpMethod::GET);
        else if (t.value == "POST")   loc.allowed_methods.insert(HttpMethod::POST);
        else if (t.value == "DELETE") loc.allowed_methods.insert(HttpMethod::DELETE);
        else
            throw std::runtime_error(
                "[config] line " + std::to_string(t.line) +
                ": unknown HTTP method '" + t.value + "'");
    }
    if (loc.allowed_methods.empty())
        throw std::runtime_error(
            "[config] line " + std::to_string(peek().line) +
            ": allowed_methods requires at least 1 method");
    expect_SEMICOLON();
}

/* grammar: autoindex_dir = "autoindex", boolean, ";" ;

boolean = "on" | "off".

when URI maps to directory and no index file exists:
    autoindex on  → server generates HTML directory listing
    autoindex off → server returns 403 Forbidden

default off: exposing filesystem structure is a security concern.
requires explicit operator opt-in. */
void Frontend::parse_autoindex(Location& loc)
{
    Token t = expect_STRING();
    if      (t.value == "on")  loc.autoindex = true;
    else if (t.value == "off") loc.autoindex = false;
    else
        throw std::runtime_error(
            "[config] line " + std::to_string(t.line) +
            ": autoindex expects 'on' or 'off', got '" + t.value + "'");
    expect_SEMICOLON();
}

/* grammar: cgi_ext_dir = "cgi_extension", extension, ";" ;

file extension that triggers CGI execution (e.g. ".py", ".php").
semantically coupled with cgi_path — validator enforces both or neither. */
void Frontend::parse_cgi_ext(Location& loc)
{
    loc.cgi_extension = expect_STRING().value;
    expect_SEMICOLON();
}

/* grammar: cgi_path_dir = "cgi_path", path, ";" ;

path to CGI interpreter (e.g. "/usr/bin/python3").
semantically coupled with cgi_extension — validator enforces. */
void Frontend::parse_cgi_path(Location& loc)
{
    loc.cgi_path = expect_STRING().value;
    expect_SEMICOLON();
}

/* grammar: body_size_dir = "client_max_body_size", size, ";" ;

location-level: wraps in std::optional.
std::nullopt (default) means inherit server-level value.
present value overrides server default for this location only.

overload resolution: this function takes Location&, the server-level
overload takes ServerConfig&. resolved at call site by argument type. */
void Frontend::parse_body_size(Location& loc)
{
    Token t = expect_STRING();
    loc.client_max_body_size = parse_size(t);
    expect_SEMICOLON();
}

/* grammar: upload_enable_dir = "upload_enable", boolean, ";" ;

enables file upload handling for this location.
semantically coupled with upload_store — validator enforces:
    upload_enable true requires upload_store non-empty. */
void Frontend::parse_upload_enable(Location& loc)
{
    Token t = expect_STRING();
    if      (t.value == "on")  loc.upload_enable = true;
    else if (t.value == "off") loc.upload_enable = false;
    else
        throw std::runtime_error(
            "[config] line " + std::to_string(t.line) +
            ": upload_enable expects 'on' or 'off', got '" + t.value + "'");
    expect_SEMICOLON();
}

/* grammar: upload_store_dir = "upload_store", path, ";" ;

filesystem path where uploaded files are written.
semantically coupled with upload_enable — validator enforces. */
void Frontend::parse_upload_store(Location& loc)
{
    loc.upload_store = expect_STRING().value;
    expect_SEMICOLON();
}

/* grammar: return_dir = "return", status_code, path, ";" ;

redirect directive. status_code in [300, 399] (redirect codes).
range enforced here (parse-time rejection) and in validator.

return_code is std::optional<uint16_t>: std::nullopt means no redirect.
semantically coupled with return_path — validator enforces:
    return_code set requires return_path non-empty. */
void Frontend::parse_return(Location& loc)
{
    Token code_tok = expect_STRING();
    Token path_tok = expect_STRING();

    int code;
    try { code = std::stoi(code_tok.value); }
    catch (...)
    {
        throw std::runtime_error(
            "[config] line " + std::to_string(code_tok.line) +
            ": invalid return code '" + code_tok.value + "'");
    }
    if (code < 300 || code > 399)
        throw std::runtime_error(
            "[config] line " + std::to_string(code_tok.line) +
            ": return code out of range [300, 399]: " + code_tok.value);

    loc.return_code = static_cast<uint16_t>(code);
    loc.return_path = path_tok.value;
    expect_SEMICOLON();
}
