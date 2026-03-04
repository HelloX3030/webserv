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

    // Handle Configs
    for (const ServerConfig &config : configs)
    {
        (void)config;
        std::cout << "config" << std::endl;
    }
}

} // namespace WebServ
