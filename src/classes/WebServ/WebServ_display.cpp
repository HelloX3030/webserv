#include "WebServ.hpp"

namespace WebServ
{

void display()
{
    logging::log(DISPLAY, WEB_SERV);
    std::size_t len = epoll_handlers.size();
    for (std::size_t i = 0; i < len; i++)
    {
        if (epoll_handlers[i])
        {
            std::cout << epoll_handlers[i]->to_string() << std::endl;
        }
    }
}

} // namespace WebServ
