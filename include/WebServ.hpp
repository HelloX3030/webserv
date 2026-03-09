#pragma once

#include "base/base.hpp"
#include "classes/Config.hpp"
#include "classes/Connection.hpp"
#include "classes/Server.hpp"

namespace WebServ
{
    extern std::vector<ServerConfig> servers;

    // Functions
    void add_test_data();
    void init();
    void parse(int argc, char **argv);
    void display();
    void run();
    void quit();

} // namespace WebServ
