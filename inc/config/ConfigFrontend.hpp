#pragma once

#include "config/Config.hpp"

#include <string>
#include <vector>


namespace ConfigFrontend
{
std::vector<ServerConfig> parse(const std::string &filepath);
}
