#include "ServerConfig.hpp"

ServerConfig::ServerConfig(std::string file_path)
{
    std::cout << "Parse ServerConfig" << file_path << std::endl;
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
