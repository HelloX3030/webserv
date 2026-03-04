#include "WebServ.hpp"

namespace WebServ
{

void display()
{
    log::log(DISPLAY, WEB_SERV);

    // EPOLL_HANDLER
    log::log(DISPLAY, EPOLL_HANDLER);
    std::size_t len = epoll_handlers.size();
    for (std::size_t i = 0; i < len; i++)
    {
        if (epoll_handlers[i])
        {
            std::cout << epoll_handlers[i]->to_string() << std::endl;
        }
    }

    // SERVER
    log::log(DISPLAY, SERVER);
    for (const Server &server : servers)
    {
        std::cout << server.to_string() << std::endl;
    }
}

} // namespace WebServ
