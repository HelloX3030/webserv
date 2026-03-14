#include "http/HttpMethods.hpp"

namespace WebServ
{

HttpResponse http_post(const ServerConfig &config, const std::string &path, const std::string &content)
{
#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, "Path=\"" + path + "\" content=\"" + content + "\"");
#endif

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
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "No matching location -> 404");
#endif
        return HttpResponse(404);
    }

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, "Matched location prefix=\"" + location_prefix + "\" root=\"" + location->root + "\"");
#endif

    // check allowed methods
    if (location->allowed_methods.count(HttpMethod::POST) == 0)
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "POST not allowed in location -> 405");
#endif
        return HttpResponse(405);
    }

    // check location body size override
    if (location->client_max_body_size.has_value())
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "Location body size limit=" + std::to_string(location->client_max_body_size.value()));
#endif

        if (content.size() > location->client_max_body_size.value())
        {
#ifdef DEBUG
            logging::log(HTTP_METHODE_POST, "Body too large -> 413");
#endif
            return HttpResponse(413);
        }
    }

    // determine base directory
    std::string base;

    if (location->upload_enable && !location->upload_store.empty())
    {
        base = location->upload_store;
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "Using upload_store base=\"" + base + "\"");
#endif
    }
    else
    {
        base = location->root;
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "Using root base=\"" + base + "\"");
#endif
    }

    // remove location prefix
    std::string relative = path.substr(location_prefix.size());
    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, "Relative path=\"" + relative + "\"");
#endif

    std::string file_path = base + "/" + relative;

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, "Constructed file_path=\"" + file_path + "\"");
#endif

    // traversal protection
    auto safe = utils::resolve_path(base, relative);

    if (!safe)
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "resolve_path rejected traversal -> 403");
#endif
        return HttpResponse(403);
    }

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, "Resolved safe path=\"" + safe->string() + "\"");
#endif

    auto parent = safe->parent_path();

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, "Parent path=\"" + parent.string() + "\"");
#endif

    if (!std::filesystem::exists(parent))
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "Parent directory does not exist -> 409");
#endif
        return HttpResponse(409);
    }

    if (std::filesystem::is_directory(*safe))
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "Target path is a directory -> 403");
#endif
        return HttpResponse(403);
    }

    bool existed = std::filesystem::exists(*safe);

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, std::string("File existed=") + (existed ? "true" : "false"));
#endif

    // write file
    std::ofstream file(safe->c_str(), std::ios::binary);

    if (!file.is_open())
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_POST, "Failed to open file for writing -> 500");
#endif
        return HttpResponse(500);
    }

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, "Writing " + std::to_string(content.size()) + " bytes");
#endif

    file.write(content.data(), content.size());
    file.close();

#ifdef DEBUG
    logging::log(HTTP_METHODE_POST, existed ? "Returning 200 OK (overwrite)" : "Returning 201 Created");
#endif

    return HttpResponse(existed ? 200 : 201);
}

} // namespace WebServ
