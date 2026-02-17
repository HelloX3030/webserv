#pragma once

#include "base/base.hpp"
#include "classes/Server.hpp"

namespace WebServ
{

extern std::vector<Server> servers;

// Functions
int init();
void parse(int argc, char **argv);
void run();
void quit();

} // namespace WebServ
