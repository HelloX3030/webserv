#include "WebServ.hpp"

void WebServ::parse(int argc, char **argv)
{
    (void)argv;
    if (argc != 1)
        throw std::runtime_error("Parsing Error :D");
    return;
}
