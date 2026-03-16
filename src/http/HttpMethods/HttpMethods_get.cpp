#include "base/defines.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "http/HttpMethods.hpp"
#include "http/HttpResponseBuilder.hpp"
#include <fstream>
#include <sstream>

namespace WebServ
{

static HttpResponseBuilder serve_file(const std::filesystem::path &file_path)
{
#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Serving file \"" + file_path.string() + "\"");
#endif

    std::ifstream file(file_path.c_str(), std::ios::binary);

    if (!file.is_open())
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "Failed to open file -> 500");
#endif
        return HttpResponseBuilder(500);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    HttpResponseBuilder res(200);
    res.set_body(buffer.str());

    res.set_content_type(file_path);

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, res.to_string());
#endif

    return res;
}

[[nodiscard]] HttpResponseBuilder http_get(const ServerConfig &config, const std::string &path)
{
#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Path=\"" + path + "\"");
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
        logging::log(HTTP_METHOD_GET, "No matching location -> 404");
#endif
        return HttpResponseBuilder(404);
    }

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Matched location prefix=\"" + location_prefix + "\" root=\"" + location->root + "\"");
#endif

    // check allowed methods
    if (location->allowed_methods.count(HttpMethod::GET) == 0)
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "GET not allowed in location -> 405");
#endif
        return HttpResponseBuilder(405);
    }

    // determine base directory
    std::string base = location->root;

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Using root base=\"" + base + "\"");
#endif

    // remove location prefix
    std::string relative = path.substr(location_prefix.size());
    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Relative path=\"" + relative + "\"");
#endif

    // traversal protection
    auto safe = utils::resolve_path(base, relative);

    if (!safe)
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "resolve_path rejected traversal -> 403");
#endif
        return HttpResponseBuilder(403);
    }

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Resolved safe path=\"" + safe->string() + "\"");
#endif

    if (!std::filesystem::exists(*safe))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "File does not exist -> 404");
#endif
        return HttpResponseBuilder(404);
    }

    // directory handling
    if (std::filesystem::is_directory(*safe))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "Target is directory");
#endif

        // try index files
        for (std::vector<std::string>::const_iterator it = location->index_files.begin();
             it != location->index_files.end(); ++it)
        {
            std::filesystem::path index_path = *safe / *it;

            if (std::filesystem::exists(index_path))
            {
#ifdef DEBUG
                logging::log(HTTP_METHOD_GET, "Serving index file \"" + index_path.string() + "\"");
#endif
                return serve_file(index_path);
            }
        }

#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "Directory without index -> 403");
#endif
        return HttpResponseBuilder(403);
    }

    // serve file
    return serve_file(*safe);
}

} // namespace WebServ
