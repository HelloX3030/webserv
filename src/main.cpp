#include "WebServ.hpp"
#include <exception>
#include <iostream>

int main(int argc, char **argv)
{
    WebServ server;
    try
    {
        server.parse(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
