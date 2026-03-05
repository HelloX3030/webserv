#include "WebServ.hpp"

namespace WebServ
{

void init(const std::vector<ServerConfig> &configs)
{

#ifdef DEBUG
    log::log(FUNCTION, "WebServ::init()");
    if (epfd != -1)
    {
        throw SetupError("Multiple Calls to WebServ::init()!");
    }
#endif

    // epoll Setup
    epfd = epoll_create1(0);
    if (epfd < 0)
    {
        throw std::system_error(errno, std::generic_category(), "epoll_create1");
    }

    // Create Servers
    servers.reserve(configs.size());
    for (const ServerConfig &config : configs)
    {
        servers.emplace_back(config);
        for (const ListenAddress &listen_adress : config.listen)
        {
            add_listener(listen_adress, config.server_names, servers[servers.size() - 1]);
        }
    }
}

} // namespace WebServ
