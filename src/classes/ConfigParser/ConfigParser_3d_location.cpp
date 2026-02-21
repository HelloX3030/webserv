#include "../../../include/classes/ConfigParser.hpp"

#include <stdexcept>
#include <string>

/* root_dir: grammar: "root", path, ";" */
void ConfigParser::parse_root(Location& loc)
{
    loc.root = expect_STRING().value;
    expect_SEMICOLON();
}

/* index_dir: grammar: "index", filename, { filename }, ";" ;
grammar requires at least 1 filename. */
void ConfigParser::parse_index(Location& loc)
{
    if (peek().type != TokenType::STRING)
        throw std::runtime_error(
            "[config] line " + std::to_string(peek().line) +
            ": index requires at least 1 filename");

    while (peek().type == TokenType::STRING)
        loc.index_files.push_back(consume().value);

    expect_SEMICOLON();
}

/* methods_dir: grammar: "allowed_methods", method, { method }, ";" ;
clears the default {GET, POST, DELETE} — this directive is an explicit
override, not additive. */
void ConfigParser::parse_methods(Location& loc)
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

/* autoindex_dir: grammar: "autoindex", boolean, ";" ;
boolean = "on" | "off" */
void ConfigParser::parse_autoindex(Location& loc)
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

/* cgi_ext_dir: grammar: "cgi_extension", extension, ";" */
void ConfigParser::parse_cgi_ext(Location& loc)
{
    loc.cgi_extension = expect_STRING().value;
    expect_SEMICOLON();
}

/* cgi_path_dir: grammar: "cgi_path", path, ";" */
void ConfigParser::parse_cgi_path(Location& loc)
{
    loc.cgi_path = expect_STRING().value;
    expect_SEMICOLON();
}

/* body_size_dir (location): grammar: "client_max_body_size", size, ";" ;
wraps in std::optional — std::nullopt means inherit server default.
present value overrides server default for this location only.
overload resolved at the call site in parse_location by argument type. */
void ConfigParser::parse_body_size(Location& loc)
{
    Token t = expect_STRING();
    loc.client_max_body_size = parse_size(t);
    expect_SEMICOLON();
}

/* upload_enable_dir: grammar: "upload_enable", boolean, ";" */
void ConfigParser::parse_upload_enable(Location& loc)
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

/* upload_store_dir: grammar: "upload_store", path, ";" */
void ConfigParser::parse_upload_store(Location& loc)
{
    loc.upload_store = expect_STRING().value;
    expect_SEMICOLON();
}

/* return_dir: grammar: "return", status_code, path, ";" ;
return_code: std::optional<uint16_t> — nullopt means no redirect.
valid range [300, 399] enforced here; validator confirms.
couplings (return_code set ↔ return_path non-empty) enforced in validator. */
void ConfigParser::parse_return(Location& loc)
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