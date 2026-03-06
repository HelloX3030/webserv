#include "classes/Server.hpp"

namespace WebServ
{

std::vector<Server> servers;

} // namespace WebServ

Server::Server(ServerConfig config)
    : config(config)
{
}

const ServerConfig &Server::get_config() const
{
    return config;
}

std::string Server::to_string() const
{
    std::string result;
    
    // Config
    result += ::to_string(config);

    return result;
}
