#include "classes/Server.hpp"

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

namespace WebServ
{

const Server* default_server = nullptr;
std::vector<Server> servers;

void add_server(const ServerConfig& config)
{
    servers.emplace_back(config);

    if (servers.size() == 1)
    {

#ifdef DEBUG
        if (default_server)
        {
            throw SetupError("Default Server is already set");
        }
#endif

        default_server = &servers[0];
    }

    for (const ListenAddress &listen_adress : config.listen)
    {
        add_listener(listen_adress, config.server_names, servers[servers.size() - 1]);
    }
}

} // namespace WebServ
