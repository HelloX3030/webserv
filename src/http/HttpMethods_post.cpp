#include "http/HttpMethods.hpp"

namespace WebServ
{

HttpResponse post(const ServerConfig &config, const std::string &path, const std::string &content)
{
    // find matching location (simplified: prefix match)
    const Location *location = NULL;
    for (std::map<std::string, Location>::const_iterator it = config.locations.begin(); it != config.locations.end(); ++it)
    {
        if (path.find(it->first) == 0)
        {
            location = &it->second;
            break;
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

    // determine filesystem path
    std::string file_path;

    if (location->upload_enable && !location->upload_store.empty())
    {
        file_path = location->upload_store + "/" + path;
    }
    else
    {
        file_path = location->root + "/" + path;
    }

    // protection against directory traversal attacks
    auto safe = utils::resolve_path(location->upload_store, file_path);
    if (!safe)
        return HttpResponse(403);

    // write file (overwrite allowed)
    std::ofstream file(safe->c_str(), std::ios::binary);
    if (!file.is_open())
        return HttpResponse(500);

    file.write(content.data(), content.size());
    file.close();

    // success response
    HttpResponse res(201);
    res.set_body("Created\n");

    return res;
}

} // namespace WebServ
