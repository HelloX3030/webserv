#pragma once

#include "Server.hpp"
#include "base.hpp"

namespace WebServ
{

extern std::vector<Server> servers;

// Functions
void parse(int argc, char **argv);
void start();

} // namespace WebServ
