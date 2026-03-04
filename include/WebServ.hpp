#pragma once

#include "base/base.hpp"
#include "classes/ConfigFrontend.hpp"
#include "classes/Connection.hpp"
#include "classes/Server.hpp"

namespace WebServ
{

// Functions
[[nodiscard]] std::vector<ServerConfig> parse(int argc, char **argv);
void init(const std::vector<ServerConfig> &configs);
void display();
void run();
void quit();

} // namespace WebServ
