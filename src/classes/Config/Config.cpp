#include "classes/Config.hpp"

std::string_view to_string(HttpMethod method)
{
    switch (method)
    {
        case HttpMethod::GET:    return "GET";
        case HttpMethod::POST:   return "POST";
        case HttpMethod::DELETE: return "DELETE";
    }

    return "UNKNOWN";
}

bool ListenAddress::operator==(const ListenAddress& other) const
{
    return host == other.host && port == other.port;
}

std::string to_string(const ListenAddress& address)
{
    return address.host + ":" + std::to_string(address.port);
}

std::string to_string(const Location& location)
{
    std::ostringstream os;

    os << "Location {\n";

    os << "  root: " << location.root << "\n";

    os << "  index_files: [";
    for (size_t i = 0; i < location.index_files.size(); ++i)
    {
        os << location.index_files[i];
        if (i + 1 < location.index_files.size())
            os << ", ";
    }
    os << "]\n";

    os << "  allowed_methods: [";
    size_t count = 0;
    for (const auto& method : location.allowed_methods)
    {
        os << to_string(method);
        if (++count < location.allowed_methods.size())
            os << ", ";
    }
    os << "]\n";

    os << "  autoindex: " << (location.autoindex ? "true" : "false") << "\n";

    os << "  cgi_extension: " << location.cgi_extension << "\n";
    os << "  cgi_path: " << location.cgi_path << "\n";

    os << "  client_max_body_size: ";
    if (location.client_max_body_size)
        os << *location.client_max_body_size;
    else
        os << "inherit";
    os << "\n";

    os << "  upload_enable: " << (location.upload_enable ? "true" : "false") << "\n";
    os << "  upload_store: " << location.upload_store << "\n";

    os << "  return: ";
    if (location.return_code)
        os << *location.return_code << " -> " << location.return_path;
    else
        os << "none";
    os << "\n";

    os << "}";

    return os.str();
}

std::string to_string(const ServerConfig& config)
{
    std::ostringstream os;

    os << "ServerConfig {\n";

    // listen
    os << "  listen: [\n";
    for (const auto& addr : config.listen)
        os << "    " << to_string(addr) << "\n";
    os << "  ]\n";

    // server names
    os << "  server_names: [";
    for (size_t i = 0; i < config.server_names.size(); ++i)
    {
        os << config.server_names[i];
        if (i + 1 < config.server_names.size())
            os << ", ";
    }
    os << "]\n";

    os << "  client_max_body_size: " << config.client_max_body_size << "\n";

    // error pages
    os << "  error_pages: {\n";
    for (const auto& [code, path] : config.error_pages)
        os << "    " << code << " -> " << path << "\n";
    os << "  }\n";

    // locations
    os << "  locations: {\n";
    for (const auto& [path, location] : config.locations)
    {
        os << "    \"" << path << "\":\n";

        std::string loc_str = to_string(location);
        std::istringstream loc_stream(loc_str);
        std::string line;

        while (std::getline(loc_stream, line))
            os << "      " << line << "\n";
    }
    os << "  }\n";

    os << "}";

    return os.str();
}
