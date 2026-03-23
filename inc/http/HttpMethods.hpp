#pragma once

#include "HttpResponseBuilder.hpp"
#include "config/Config.hpp"

class Connection;

namespace WebServ
{

[[nodiscard]] HttpResponseBuilder http_get(const std::filesystem::path &resolved_path, const std::vector<std::string> &index_files);
[[nodiscard]] HttpResponseBuilder http_post(const std::filesystem::path &resolved_path, const std::string &content);
[[nodiscard]] HttpResponseBuilder http_delete(const std::filesystem::path &resolved_path);
[[nodiscard]] HttpResponseBuilder http_handle_request(const Connection &connection, HttpMethod method, std::string target, std::map<std::string, std::string> headers, std::string body);

} // namespace WebServ
