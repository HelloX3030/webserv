#include "http/HttpMethods.hpp"

namespace WebServ
{

HttpResponse post(const ServerConfig &config, const std::string &path, const std::string &content)
{
    log::log(HTTP_METHODE_POST, "Path=\"" + path + "\" content=\"" + content + "\"");

    // find matching location (longest prefix match)
    const Location *location = NULL;
    std::string location_prefix;

    for (std::map<std::string, Location>::const_iterator it = config.locations.begin();
         it != config.locations.end(); ++it)
    {
        if (path.find(it->first) == 0)
        {
            if (it->first.size() > location_prefix.size())
            {
                location = &it->second;
                location_prefix = it->first;
            }
        }
    }

    if (!location)
    {
        log::log(HTTP_METHODE_POST, "No matching location -> 404");
        return HttpResponse(404);
    }

    log::log(HTTP_METHODE_POST, "Matched location prefix=\"" + location_prefix + "\" root=\"" + location->root + "\"");

    // check allowed methods
    if (location->allowed_methods.count(HttpMethod::POST) == 0)
    {
        log::log(HTTP_METHODE_POST, "POST not allowed in location -> 405");
        return HttpResponse(405);
    }

    // check location body size override
    if (location->client_max_body_size.has_value())
    {
        log::log(HTTP_METHODE_POST, "Location body size limit=" + std::to_string(location->client_max_body_size.value()));

        if (content.size() > location->client_max_body_size.value())
        {
            log::log(HTTP_METHODE_POST, "Body too large -> 413");
            return HttpResponse(413);
        }
    }

    // determine base directory
    std::string base;

    if (location->upload_enable && !location->upload_store.empty())
    {
        base = location->upload_store;
        log::log(HTTP_METHODE_POST, "Using upload_store base=\"" + base + "\"");
    }
    else
    {
        base = location->root;
        log::log(HTTP_METHODE_POST, "Using root base=\"" + base + "\"");
    }

    // remove location prefix
    std::string relative = path.substr(location_prefix.size());
    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

    log::log(HTTP_METHODE_POST, "Relative path=\"" + relative + "\"");

    std::string file_path = base + "/" + relative;
    log::log(HTTP_METHODE_POST, "Constructed file_path=\"" + file_path + "\"");

    // traversal protection
    auto safe = utils::resolve_path(base, relative);

    if (!safe)
    {
        log::log(HTTP_METHODE_POST, "resolve_path rejected traversal -> 403");
        return HttpResponse(403);
    }

    log::log(HTTP_METHODE_POST, "Resolved safe path=\"" + safe->string() + "\"");

    bool existed = std::filesystem::exists(*safe);
    log::log(HTTP_METHODE_POST, std::string("File existed=") + (existed ? "true" : "false"));

    // write file
    std::ofstream file(safe->c_str(), std::ios::binary);

    if (!file.is_open())
    {
        log::log(HTTP_METHODE_POST, "Failed to open file for writing -> 500");
        return HttpResponse(500);
    }

    log::log(HTTP_METHODE_POST, "Writing " + std::to_string(content.size()) + " bytes");

    file.write(content.data(), content.size());
    file.close();

    log::log(HTTP_METHODE_POST, existed ? "Returning 200 OK (overwrite)" : "Returning 201 Created");

    return HttpResponse(existed ? 200 : 201);
}

} // namespace WebServ
