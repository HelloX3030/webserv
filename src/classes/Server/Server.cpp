#include "classes/Server.hpp"

namespace WebServ
{

std::vector<Server> servers;

} // namespace WebServ

Server::Server(ServerConfig config)
    : config(config)
{
}

std::string Server::to_string() const
{
    std::string result = "    ";
    for (auto &name : config.server_names)
    {
        result += name + " ";
    }
    return result;
}
