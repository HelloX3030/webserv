#include "WebServ.hpp"

namespace WebServ
{

void quit()
{
    for (std::size_t fd = 0; fd < epoll_handlers.size(); fd++)
    {
        remove_epoll_handler(fd);
    }
}

} // namespace WebServ
