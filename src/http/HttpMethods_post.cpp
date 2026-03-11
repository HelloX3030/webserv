#include "http/HttpMethods.hpp"

namespace WebServ
{

HttpResponse post(const ServerConfig &config, const std::string &path, const std::string &content)
{
    // find matching location (simplified: prefix match)
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
        return HttpResponse(404);

    // check allowed methods
    if (location->allowed_methods.count(HttpMethod::POST) == 0)
        return HttpResponse(405);

    // check location body size override
    if (location->client_max_body_size.has_value())
    {
        if (content.size() > location->client_max_body_size.value())
            return HttpResponse(413);
    }

    // determine base directory
    std::string base;

    if (location->upload_enable && !location->upload_store.empty())
        base = location->upload_store;
    else
        base = location->root;

    // remove location prefix
    std::string relative = path.substr(location_prefix.size());
    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

    std::string file_path = base + "/" + relative;

    // traversal protection
    auto safe = utils::resolve_path(base, file_path);
    if (!safe)
        return HttpResponse(403);

    bool existed = std::filesystem::exists(*safe);

    // write file
    std::ofstream file(safe->c_str(), std::ios::binary);
    if (!file.is_open())
        return HttpResponse(500);

    file.write(content.data(), content.size());
    file.close();

    return HttpResponse(existed ? 200 : 201);
}

} // namespace WebServ
