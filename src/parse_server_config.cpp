#include "Server.hpp"

void Server::parse(const std::string &path)
{
    log::log("Parse ServerConfig", path);
}

void Server::parse_args(int argc, char **argv)
{
    Server server;

    std::string config_path;
    if (argc == 1)
    {
        config_path = "default/path";
    }
    else if (argc == 2)
    {
        config_path = argv[1];
    }
    else
    {
        throw std::runtime_error("To many args!");
    }
    server.parse(config_path);
}
