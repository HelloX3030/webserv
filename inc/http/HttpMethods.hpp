#pragma once

#include "HttpResponseBuilder.hpp"
#include "config/Config.hpp"

class Connection;
struct HttpRequest;

namespace WebServ
{

[[nodiscard]] HttpResponseBuilder http_get(const std::filesystem::path &resolved_path, const std::vector<std::string> &index_files);
[[nodiscard]] HttpResponseBuilder http_post(const std::filesystem::path &resolved_path, const std::string &content);
[[nodiscard]] HttpResponseBuilder http_delete(const std::filesystem::path &resolved_path);
[[nodiscard]] HttpResponseBuilder http_cgi(const std::filesystem::path &script_path, const std::string &interpreter, HttpMethod method, const std::string &target, const std::map<std::string, std::string> &headers, const std::string &body);
[[nodiscard]] HttpResponseBuilder http_handle_request(const Connection &connection, const HttpRequest &request);

} // namespace WebServ
