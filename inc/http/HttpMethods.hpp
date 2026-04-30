#pragma once

#include "HttpResponseBuilder.hpp"
#include "config/Config.hpp"

class Connection;
struct HttpRequest;

/* Request handlers and dispatcher.

Misnamed: this file contains per-method handlers (http_get, http_post,
http_delete), the CGI delegation mechanism, and the request dispatcher
(http_handle_request) — none of which are "HTTP methods" proper.
The HTTP methods themselves (the protocol vocabulary) are in Config.hpp
(`HttpMethod` enum and `http_method_from_string`).

For redo: split into RequestHandlers, RequestDispatch, Cgi. */
namespace WebServ
{

[[nodiscard]] HttpResponseBuilder http_get(const std::filesystem::path &resolved_path, const std::vector<std::string> &index_files, bool autoindex, const std::string &request_path);
[[nodiscard]] HttpResponseBuilder http_post(const std::filesystem::path &resolved_path, const std::string &content);
[[nodiscard]] HttpResponseBuilder http_delete(const std::filesystem::path &resolved_path);
[[nodiscard]] HttpResponseBuilder http_cgi(const std::filesystem::path &script_path, const std::string &interpreter, HttpMethod method, const std::string &target, const std::map<std::string, std::string> &headers, const std::string &body);
[[nodiscard]] HttpResponseBuilder http_handle_request(const Connection &connection, const HttpRequest &request);

} // namespace WebServ
