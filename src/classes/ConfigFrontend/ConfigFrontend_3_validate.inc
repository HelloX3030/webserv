/* validate the completed std::vector<ServerConfig>.

2-layer enforcement strategy for constrained values:

layer 1 — parse-time rejection:
    directive helpers (parse_port, parse_error_page, parse_return)
    call std::stoi, range-check immediately, throw with line number.
    illegal value never enters the struct.

layer 2 — validator confirmation:
    checks all constrained fields in completed structs.
    belt-and-suspenders: ensures no code path bypassed layer 1.

design decision: 2-layer enforcement chosen over wrapper types
(e.g. HttpStatusCode with throwing constructor). wrapper types
add boilerplate without proportionate benefit at this scale.
enforcement is explicit; sites are documented.

constraints checked here:

    mandatory presence:
        server: at least 1 listen address
        server: at least 1 location block
        location: root directive

    value ranges:
        port: [1, 65535]
        error_page code: [100, 599]
        return_code: [300, 399]
        client_max_body_size: > 0

    semantic couplings:
        cgi_extension ↔ cgi_path: both or neither
        upload_enable ↔ upload_store: enable=true requires store non-empty
        return_code ↔ return_path: code set requires path non-empty */
void ConfigFrontend::validate(const std::vector<ServerConfig>& servers)
{
    if (servers.empty())
        throw std::runtime_error(
            "[config] validation error: no server block defined");

    for (const ServerConfig& s : servers)
        validate_server(s);
}

/* mandatory fields and per-location delegation. */
void ConfigFrontend::validate_server(const ServerConfig& s)
{
    if (s.listen.empty())
        throw std::runtime_error(
            "[config] validation error: server block has no listen directive");

    if (s.locations.empty())
        throw std::runtime_error(
            "[config] validation error: server block has no location block");

    if (s.client_max_body_size == 0)
        throw std::runtime_error(
            "[config] validation error: client_max_body_size must be > 0");

    for (const auto& [code, path] : s.error_pages)
    {
        if (code < 100 || code > 599)
            throw std::runtime_error(
                "[config] validation error: error_page code out of range"
                " — got " + std::to_string(code) + ", valid range [100, 599]");
    }

    for (const auto& addr : s.listen)
    {
        if (addr.port < 1)
            throw std::runtime_error(
                "[config] validation error: listen port out of range"
                " — got " + std::to_string(addr.port) + ", valid range [1, 65535]");
    }

    for (const auto& [path, loc] : s.locations)
        validate_location(path, loc);
}

/* mandatory fields and semantic couplings.

root: mandatory. without it the server cannot resolve any file path.
grammatically optional (location_dir = root_dir | ...), semantically required.

cgi coupling: cgi_extension and cgi_path are co-dependent.
server cannot invoke CGI without both extension and interpreter path.

upload coupling: upload_enable true requires upload_store non-empty.
enabling upload without destination path is incomplete.

return coupling: return_code set requires return_path non-empty.
redirect without target URI is malformed. */
void ConfigFrontend::validate_location(const std::string& path, const Location& loc)
{
    if (loc.root.empty())
        throw std::runtime_error(
            "[config] validation error: location '" + path + "' has no root directive");

    bool has_cgi_ext  = !loc.cgi_extension.empty();
    bool has_cgi_path = !loc.cgi_path.empty();
    if (has_cgi_ext != has_cgi_path)
        throw std::runtime_error(
            "[config] validation error: location '" + path + "': "
            "cgi_extension and cgi_path must both be set or both absent");

    if (loc.upload_enable && loc.upload_store.empty())
        throw std::runtime_error(
            "[config] validation error: location '" + path + "': "
            "upload_enable requires upload_store");

    bool has_return_code = loc.return_code.has_value();
    bool has_return_path = !loc.return_path.empty();
    if (has_return_code != has_return_path)
        throw std::runtime_error(
            "[config] validation error: location '" + path + "': "
            "return_code and return_path must both be set or both absent");

    if (has_return_code && (*loc.return_code < 300 || *loc.return_code > 399))
        throw std::runtime_error(
            "[config] validation error: location '" + path + "': "
            "return_code out of range [300, 399]");
}
