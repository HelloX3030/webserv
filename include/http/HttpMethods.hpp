#pragma once

#include "HttpResponse.hpp"
#include "classes/Config.hpp"

namespace WebServ
{

[[nodiscard]] HttpResponse http_get(const ServerConfig &config, const std::string &path);
[[nodiscard]] HttpResponse http_post(const ServerConfig &config, const std::string &path, const std::string &content);
[[nodiscard]] HttpResponse http_delete(const ServerConfig &config, const std::string &path);

} // namespace WebServ
