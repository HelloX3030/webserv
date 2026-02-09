#include "WebServ.hpp"
#include <exception>
#include <iostream>

int main(int argc, char **argv)
{
    WebServ server;

    // Parsing
    try
    {
        server.parse(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // Run Server

    return 0;
}
