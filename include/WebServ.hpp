#pragma once

#include "config/Config.hpp"
#include "core/Server.hpp"
#include "core/signal.hpp"
#include "net/Connection.hpp"

namespace WebServ
{
// Functions
[[nodiscard]] std::vector<ServerConfig> load_config(int argc, char **argv);
void init(const std::vector<ServerConfig> &configs);
void display();
void run();
void quit();

} // namespace WebServ
