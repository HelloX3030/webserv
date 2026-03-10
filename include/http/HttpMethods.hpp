#pragma once

#include "HttpResponse.hpp"
#include "classes/Config.hpp"

namespace WebServ
{

[[nodiscard]] HttpResponse post(const ServerConfig &config, const std::string &path, const std::string &content);

}
