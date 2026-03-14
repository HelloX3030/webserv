#include "http/HttpMethods.hpp"

namespace WebServ
{

[[nodiscard]] HttpResponse http_delete(const ServerConfig &config, const std::string &path)
{
#ifdef DEBUG
    logging::log(HTTP_METHODE_DELETE, "Path=\"" + path + "\"");
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
        logging::log(HTTP_METHODE_DELETE, "No matching location -> 404");
#endif
        return HttpResponse(404);
    }

#ifdef DEBUG
    logging::log(HTTP_METHODE_DELETE, "Matched location prefix=\"" + location_prefix + "\" root=\"" + location->root + "\"");
#endif

    // check allowed methods
    if (location->allowed_methods.count(HttpMethod::DELETE) == 0)
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_DELETE, "DELETE not allowed -> 405");
#endif
        return HttpResponse(405);
    }

    std::string base = location->root;

#ifdef DEBUG
    logging::log(HTTP_METHODE_DELETE, "Using root base=\"" + base + "\"");
#endif

    // remove location prefix
    std::string relative = path.substr(location_prefix.size());
    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

#ifdef DEBUG
    logging::log(HTTP_METHODE_DELETE, "Relative path=\"" + relative + "\"");
#endif

    // traversal protection
    auto safe = utils::resolve_path(base, relative);

    if (!safe)
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_DELETE, "resolve_path rejected traversal -> 403");
#endif
        return HttpResponse(403);
    }

#ifdef DEBUG
    logging::log(HTTP_METHODE_DELETE, "Resolved safe path=\"" + safe->string() + "\"");
#endif

    if (!std::filesystem::exists(*safe))
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_DELETE, "File does not exist -> 404");
#endif
        return HttpResponse(404);
    }

    if (std::filesystem::is_directory(*safe))
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_DELETE, "Target is directory -> 403");
#endif
        return HttpResponse(403);
    }

#ifdef DEBUG
    logging::log(HTTP_METHODE_DELETE, "Deleting file");
#endif

    try
    {
        std::filesystem::remove(*safe);
    }
    catch (...)
    {
#ifdef DEBUG
        logging::log(HTTP_METHODE_DELETE, "Filesystem deletion failed -> 500");
#endif
        return HttpResponse(500);
    }

#ifdef DEBUG
    logging::log(HTTP_METHODE_DELETE, "File deleted -> 200");
#endif

    return HttpResponse(200);
}

} // namespace WebServ
