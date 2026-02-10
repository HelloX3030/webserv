#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
{
}

ServerConfig::ServerConfig(const ServerConfig &other)
{
    *this = other;
}

ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    if (this != &other)
    {
    }
    return *this;
}

ServerConfig::~ServerConfig()
{
}

void ServerConfig::parse(const std::string &path)
{
    log::log("Parse ServerConfig", path);
}

void ServerConfig::parse_args(int argc, char **argv)
{
    ServerConfig server_config;

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
    server_config.parse(config_path);
}
