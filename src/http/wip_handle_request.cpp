#include "base/defines.hpp"
#include "base/utils.hpp"
#include "http/HttpMethods.hpp"
#include "net/Connection.hpp"

namespace WebServ
{

// TODO => move it to proper location + connect it with HttpRequestFrontend
HttpResponseBuilder wip_handle_request(const Connection &connection, HttpMethod method, std::string target, std::map<std::string, std::string> headers, std::string body)
{
    // select server config
    const ServerConfig *config = &connection.get_default_server_config();

    auto it = headers.find(HOST);
    if (it != headers.end())
    {
        std::string host = it->second;

        // strip port if present
        size_t pos = host.find(':');
        if (pos != std::string::npos)
            host = host.substr(0, pos);

        config = &connection.get_server_config(host);
    }

    // parse target → path
    std::string path;
    size_t qpos = target.find('?');

    if (qpos == std::string::npos)
        path = target;
    else
        path = target.substr(0, qpos);

    // match location (longest prefix)
    const Location *location = NULL;
    std::string location_prefix;
    for (std::map<std::string, Location>::const_iterator it = config->locations.begin(); it != config->locations.end(); ++it)
    {
        const std::string &prefix = it->first;

        if (path.compare(0, prefix.size(), prefix) == 0 && (path.size() == prefix.size() || path[prefix.size()] == '/'))
        {
            if (prefix.size() > location_prefix.size())
            {
                location = &it->second;
                location_prefix = prefix;
            }
        }
    }

    if (!location)
        return HttpResponseBuilder(404);

    // check allowed methods
    if (location->allowed_methods.count(method) == 0)
        return HttpResponseBuilder(405);

    // check body size (POST)
    if (method == HttpMethod::POST && location->client_max_body_size.has_value() && body.size() > location->client_max_body_size.value())
    {
        return HttpResponseBuilder(413);
    }

    // determine base directory
    std::string base;
    if (method == HttpMethod::POST && location->upload_enable && !location->upload_store.empty())
    {
        base = location->upload_store;
    }
    else
    {
        base = location->root;
    }

    // build relative path
    std::string relative = path.substr(location_prefix.size());

    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

    // resolve safe filesystem path
    auto safe = utils::resolve_path(base, relative);

    if (!safe)
        return HttpResponseBuilder(403);

    // TODO: CGI or static (placeholder)

    // dispatch by method
    switch (method)
    {
    case HttpMethod::GET:
        return http_get(*config, safe->string());

    case HttpMethod::POST:
        return http_post(*safe, body);

    case HttpMethod::DELETE:
        return http_delete(*config, safe->string());

    default:
        return HttpResponseBuilder(405);
    }
}

} // namespace WebServ
