#pragma once

#include "HttpResponseBuilder.hpp"
#include "config/Config.hpp"

class Connection;

namespace WebServ
{

[[nodiscard]] HttpResponseBuilder http_get(const ServerConfig &config, const std::string &path);
[[nodiscard]] HttpResponseBuilder http_post(const std::filesystem::path &resolved_path, const std::string &content);
[[nodiscard]] HttpResponseBuilder http_delete(const ServerConfig &config, const std::string &path);
[[nodiscard]] HttpResponseBuilder wip_handle_request(const Connection &connection, HttpMethod method, std::string target, std::map<std::string, std::string> headers, std::string body);

} // namespace WebServ
