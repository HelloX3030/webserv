#include "../../../include/classes/ConfigFrontend.hpp"

#include <stdexcept>
#include <string>

/* validate the completed std::vector<ServerConfig>.
check semantic constraints the grammar cannot express & 
the parser cannot check locally — 
constraints that require fully built struct.

2-layer enforcement strategy:
parse time: range checks on numeric values (port, status code, size).
  illegal values rejected before they enter the structs.
validate time: confirms those constraints held across every code path,
  checks mandatory field presence, and checks semantic couplings between
  fields that have no individual syntax error but are invalid in combination. */
void ConfigFrontend::validate(const std::vector<ServerConfig>& servers)
{
    if (servers.empty())
        throw std::runtime_error(
            "[config] validation error: no server block defined");

    for (const ServerConfig& s : servers)
        validate_server(s);
}

/* mandatory fields & per-location checks */
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
        if (addr.port < 1 || addr.port > 65535)
            throw std::runtime_error(
                "[config] validation error: listen port out of range"
                " — got " + std::to_string(addr.port) + ", valid range [1, 65535]");
    }

    for (const auto& [path, loc] : s.locations)
        validate_location(path, loc);
}

/* mandatory fields and semantic couplings.

root: mandatory — without it the server cannot resolve any file path.
  the validator enforces this rather than the parser because root
  is grammatically optional (location_dir = root_dir | ...) and
  absence is only an error in the completed struct.

cgi coupling: cgi_extension and cgi_path are co-dependent.
  either both must be set or both must be absent. server cannot invoke CGI
  without both an extension to trigger on and a path to the interpreter.

upload coupling: upload_enable true requires upload_store non-empty.
  enabling upload without a destination path is an incomplete directive.

return coupling: return_code set requires return_path non-empty.
  a redirect without a target URI is malformed. */
void ConfigFrontend::validate_location(const std::string& path,
                                     const Location& loc)
{
    if (loc.root.empty())
        throw std::runtime_error(
            "[config] validation error: location '" + path +
            "' has no root directive");

    bool has_cgi_ext  = !loc.cgi_extension.empty();
    bool has_cgi_path = !loc.cgi_path.empty();
    if (has_cgi_ext != has_cgi_path)
        throw std::runtime_error(
            "[config] validation error: location '" + path +
            "': cgi_extension and cgi_path must both be set or both absent");

    if (loc.upload_enable && loc.upload_store.empty())
        throw std::runtime_error(
            "[config] validation error: location '" + path +
            "': upload_enable is on but upload_store is not set");

    if (loc.return_code.has_value() && loc.return_path.empty())
        throw std::runtime_error(
            "[config] validation error: location '" + path +
            "': return code set but return path is empty");
}