#pragma once

#include "HttpResponse.hpp"
#include "classes/Config.hpp"

namespace WebServ
{

[[nodiscard]] HttpResponse get(const ServerConfig &config, const std::string &path);
[[nodiscard]] HttpResponse post(const ServerConfig &config, const std::string &path, const std::string &content);

} // namespace WebServ
