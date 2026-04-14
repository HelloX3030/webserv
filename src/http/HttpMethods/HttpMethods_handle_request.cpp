#include "base/defines.hpp"
#include "base/format.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "config/Config.hpp"
#include "http/HttpMethods.hpp"
#include "http/HttpRequest.hpp"
#include "net/Connection.hpp"
#include <fstream>
#include <sstream>

namespace
{

void apply_error_page_if_configured(const ServerConfig &config, HttpResponseBuilder &response)
{
    uint16_t status_code = response.get_status_code();

    if (status_code < 400 || status_code > 599)
        return;

    std::map<uint16_t, std::string>::const_iterator ep = config.error_pages.find(status_code);
    if (ep == config.error_pages.end())
        return;

    const std::string &error_uri = ep->second;

    utils::LocationMatch error_match = utils::match_location(config, error_uri);
    if (!error_match.location)
        return;

    std::string relative = error_uri.substr(error_match.prefix.size());
    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

    std::optional<std::filesystem::path> safe = utils::resolve_path(error_match.location->root, relative);
    if (!safe)
        return;

    if (!std::filesystem::exists(*safe) || std::filesystem::is_directory(*safe))
        return;

    std::ifstream file(safe->c_str(), std::ios::binary);
    if (!file.is_open())
        return;

    std::stringstream buffer;
    buffer << file.rdbuf();

    response.set_body(buffer.str());
    response.set_content_type(*safe);
}

} // namespace

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

    auto finalize_response = [&](HttpResponseBuilder response) -> HttpResponseBuilder
    {
        apply_error_page_if_configured(*config, response);
        return response;
    };

    if (!match.location)
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "No matching location -> 404");
#endif
        return finalize_response(HttpResponseBuilder(HttpStatus::NotFound));
    }

    // HEAD is parsed as a recognized HTTP token but not implemented by this server.
    // Return 405 for matched locations to align with method restriction semantics.
    if (request.method == "HEAD")
        return finalize_response(HttpResponseBuilder(HttpStatus::MethodNotAllowed));

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
        return finalize_response(response);
    }

    // check allowed methods
    if (match.location->allowed_methods.count(method) == 0)
    {
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Method not allowed -> 405");
#endif
        return finalize_response(HttpResponseBuilder(HttpStatus::MethodNotAllowed));
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
            return finalize_response(HttpResponseBuilder(HttpStatus::PayloadTooLarge));
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
        return finalize_response(HttpResponseBuilder(HttpStatus::Forbidden));
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
        return finalize_response(http_cgi(*safe, match.location->cgi_path, method, path, request.headers, request.body));
    }

    // dispatch by method
    switch (method)
    {
    case HttpMethod::GET:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Calling http_get");
#endif
        return finalize_response(http_get(*safe, match.location->index_files));

    case HttpMethod::POST:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Calling http_post");
#endif
        return finalize_response(http_post(*safe, request.body));

    case HttpMethod::DELETE:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Calling http_delete");
#endif
        return finalize_response(http_delete(*safe));

    default:
#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Unsupported method -> 405");
#endif
        return finalize_response(HttpResponseBuilder(HttpStatus::MethodNotAllowed));
    }
}

} // namespace WebServ
