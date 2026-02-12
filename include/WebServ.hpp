#pragma once

#include "base/base.hpp"
#include "classes/Server.hpp"

namespace WebServ
{

extern std::vector<Server> servers;

// Functions
void parse(int argc, char **argv);
void start();

} // namespace WebServ
