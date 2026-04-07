#include "base/defines.hpp"
#include "base/format.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "config/Config.hpp"
#include "http/HttpMethods.hpp"
#include "http/HttpRequest.hpp"
#include "net/Connection.hpp"

namespace WebServ
{

[[nodiscard]] HttpResponseBuilder http_handle_request(const Connection &connection, const HttpRequest &request)
{
#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "");
    std::cout << format::header("http_handle_request") << std::endl;
    std::cout << request.to_string();
    std::cout << format::header("http_handle_request") << std::endl;
#endif

    HttpMethod method = http_methode_from_string(request.method);

    // select server config
    const ServerConfig *config = &connection.get_default_server_config();

#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "Using default server config");
#endif

    auto it = request.headers.find("host");
    if (it != request.headers.end())
    {
        std::string host = it->second;

#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Host header=\"" + host + "\"");
#endif

        // strip port if present
        size_t pos = host.find(':');
        if (pos != std::string::npos)
        {
            host = host.substr(0, pos);
#ifdef DEBUG
            logging::log(HANDLE_REQUEST, "Stripped host=\"" + host + "\"");
#endif
        }

        config = &connection.get_server_config(host);

#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Selected server config for host=\"" + host + "\"");
#endif
    }

    // parse target → path
    std::string path;
    size_t qpos = request.uri.find('?');

    if (qpos == std::string::npos)
        path = request.uri;
    else
        path = request.uri.substr(0, qpos);

#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "Parsed path=\"" + path + "\"");
#endif

    utils::LocationMatch match = utils::match_location(*config, path);

    if (!match.location)
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "No matching location -> 404");
#endif
        return HttpResponseBuilder(404);
    }

#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "Selected location prefix=\"" + match.prefix + "\" root=\"" + match.location->root + "\"");
#endif

    // location redirect (return code + location)
    if (match.location->return_code.has_value())
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST,
                     "Applying redirect -> " + std::to_string(*match.location->return_code) +
                         " location=\"" + match.location->return_path + "\"");
#endif

        HttpResponseBuilder response(*match.location->return_code);
        response.set_header("Location", match.location->return_path);
        return response;
    }

    // check allowed methods
    if (match.location->allowed_methods.count(method) == 0)
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Method not allowed -> 405");
#endif
        return HttpResponseBuilder(405);
    }

#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "Method allowed");
#endif

    // check body size (POST)
    if (method == HttpMethod::POST && match.location->client_max_body_size.has_value())
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Body size=" + std::to_string(request.body.size()) + " limit=" + std::to_string(match.location->client_max_body_size.value()));
#endif

        if (request.body.size() > match.location->client_max_body_size.value())
        {
#ifdef DEBUG
            logging::log(HANDLE_REQUEST, "Body too large -> 413");
#endif
            return HttpResponseBuilder(413);
        }
    }

    // determine base directory
    std::string base;

    if (method == HttpMethod::POST && match.location->upload_enable && !match.location->upload_store.empty())
    {
        base = match.location->upload_store;
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Using upload_store base=\"" + base + "\"");
#endif
    }
    else
    {
        base = match.location->root;
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Using root base=\"" + base + "\"");
#endif
    }

    // build relative path
    std::string relative = path.substr(match.prefix.size());

    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "Relative path=\"" + relative + "\"");
#endif

    // resolve safe filesystem path
    auto safe = utils::resolve_path(base, relative);

    if (!safe)
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "resolve_path rejected traversal -> 403");
#endif
        return HttpResponseBuilder(403);
    }

#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "Resolved path=\"" + safe->string() + "\"");
#endif

#ifdef DEBUG
    logging::log(HANDLE_REQUEST, "Checking CGI");
#endif

    // handle cgi
    bool is_cgi = !match.location->cgi_extension.empty() && safe->extension() == match.location->cgi_extension;
    if (is_cgi)
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Dispatching to CGI");
#endif
        return http_cgi(*safe, match.location->cgi_path, method, path, request.headers, request.body);
    }

    // dispatch by method
    switch (method)
    {
    case HttpMethod::GET:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Calling http_get");
#endif
        return http_get(*safe, match.location->index_files);

    case HttpMethod::POST:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Calling http_post");
#endif
        return http_post(*safe, request.body);

    case HttpMethod::DELETE:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Calling http_delete");
#endif
        return http_delete(*safe);

    default:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Unsupported method -> 405");
#endif
        return HttpResponseBuilder(405);
    }
}

} // namespace WebServ
