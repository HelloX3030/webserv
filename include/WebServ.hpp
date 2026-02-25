#pragma once

#include "base/base.hpp"
#include "classes/Connection.hpp"
#include "classes/Server.hpp"

namespace WebServ
{

// Functions
void add_test_data();
int init();
void parse(int argc, char **argv);
void display();
int run();
void quit();

} // namespace WebServ
