#include "config/Config.hpp"

#include <ostream>
#include <sstream>
#include <string>

bool is_http_method(const std::string &str)
{
    return str == "GET" ||
           str == "POST" ||
           str == "DELETE";
}

HttpMethod http_method_from_string(const std::string &str)
{
    if (str == "GET")
        return HttpMethod::GET;
    else if (str == "POST")
        return HttpMethod::POST;
    else
        return HttpMethod::DELETE;
}

/*
display serialisation for Config data types.

programmer-facing: complete field coverage, labelled and indented.
does not reconstruct valid nginx syntax — renders state for inspection.

2 access points, 1 rendering path:
    to_string(x)     → std::string, for embedding in log messages or errors.
    operator<<(os, x) → delegates to to_string. single definition, no divergence.

maintenance liability: each to_string function manually enumerates
its struct's fields. C++17 has no reflection — the compiler cannot
detect a field added to a struct but omitted here. divergence is
silent. update to_string whenever the corresponding struct changes.
*/

std::string to_string(HttpMethod m)
{
    switch (m)
    {
    case HttpMethod::GET:
        return "GET";
    case HttpMethod::POST:
        return "POST";
    case HttpMethod::DELETE:
        return "DELETE";
    }
    return "?";
}

bool ListenAddress::operator==(const ListenAddress &other) const
{
    return host == other.host && port == other.port;
}

/*
format mirrors the listen directive syntax: host:port.
e.g. "0.0.0.0:8080", "127.0.0.1:3000"
*/
std::string to_string(const ListenAddress &addr)
{
    return addr.host + ":" + std::to_string(addr.port);
}

std::ostream &operator<<(std::ostream &os, const ListenAddress &addr)
{
    return os << to_string(addr);
}

/*
Location has no self-knowledge of its map key (the path prefix).
the key lives in ServerConfig's locations map. to_string(Location)
renders block contents only — the composite renderer in
to_string(ServerConfig) provides path context.

client_max_body_size rendered as "(inherit)" when std::nullopt —
distinguishes "not set" from a numeric value, which a bare integer
cannot express.
*/
std::string to_string(const Location &loc)
{
    std::ostringstream os;

    os << "Location {\n";
    os << "  root:                 " << loc.root << "\n";

    os << "  index_files:          [";
    for (size_t i = 0; i < loc.index_files.size(); ++i)
    {
        if (i)
            os << ", ";
        os << loc.index_files[i];
    }
    os << "]\n";

    os << "  allowed_methods:      {";
    bool first = true;
    for (HttpMethod m : loc.allowed_methods)
    {
        if (!first)
            os << " ";
        os << to_string(m);
        first = false;
    }
    os << "}\n";

    os << "  autoindex:            " << (loc.autoindex ? "on" : "off") << "\n";

    os << "  cgi_extension:        "
       << (loc.cgi_extension.empty() ? "(none)" : loc.cgi_extension) << "\n";
    os << "  cgi_path:             "
       << (loc.cgi_path.empty() ? "(none)" : loc.cgi_path) << "\n";

    os << "  client_max_body_size: ";
    if (loc.client_max_body_size.has_value())
        os << loc.client_max_body_size.value();
    else
        os << "(inherit)";
    os << "\n";

    os << "  upload_enable:        " << (loc.upload_enable ? "on" : "off") << "\n";
    os << "  upload_store:         "
       << (loc.upload_store.empty() ? "(none)" : loc.upload_store) << "\n";

    os << "  return:               ";
    if (loc.return_code.has_value())
        os << loc.return_code.value() << " " << loc.return_path;
    else
        os << "(none)";
    os << "\n";

    os << "}";
    return os.str();
}

std::ostream &operator<<(std::ostream &os, const Location &loc)
{
    return os << to_string(loc);
}

/*
indentation of nested Location blocks is the responsibility of this
composite renderer, not of to_string(Location) itself — keeps the
inner function self-contained and independently usable.

getline loop re-indents each line of the Location block by 6 spaces,
aligning it under the path key it belongs to.
*/
std::string to_string(const ServerConfig &cfg)
{
    std::ostringstream os;

    os << "ServerConfig {\n";

    os << "  listen:\n";
    for (const auto &addr : cfg.listen)
        os << "    " << to_string(addr) << "\n";

    os << "  server_names:         [";
    for (size_t i = 0; i < cfg.server_names.size(); ++i)
    {
        if (i)
            os << ", ";
        os << cfg.server_names[i];
    }
    os << "]\n";

    os << "  client_max_body_size: " << cfg.client_max_body_size << "\n";

    os << "  error_pages:\n";
    if (cfg.error_pages.empty())
        os << "    (none)\n";
    else
        for (const auto &[code, path] : cfg.error_pages)
            os << "    " << code << " -> " << path << "\n";

    os << "  locations:\n";
    for (const auto &[path, loc] : cfg.locations)
    {
        os << "    \"" << path << "\" ->\n";
        std::istringstream lines(to_string(loc));
        std::string line;
        while (std::getline(lines, line))
            os << "      " << line << "\n";
    }

    os << "}";
    return os.str();
}

std::ostream &operator<<(std::ostream &os, const ServerConfig &cfg)
{
    return os << to_string(cfg);
}
