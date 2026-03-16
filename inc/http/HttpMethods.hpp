#pragma once

#include "HttpResponseBuilder.hpp"
#include "config/Config.hpp"

namespace WebServ
{

[[nodiscard]] HttpResponseBuilder http_get(const ServerConfig &config, const std::string &path);
[[nodiscard]] HttpResponseBuilder http_post(const ServerConfig &config, const std::string &path, const std::string &content);
[[nodiscard]] HttpResponseBuilder http_delete(const ServerConfig &config, const std::string &path);

} // namespace WebServ
