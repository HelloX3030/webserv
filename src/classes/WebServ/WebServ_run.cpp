#include "WebServ.hpp"

namespace WebServ
{

int run()
{
    struct epoll_event events[WEBSERV_EPOLL_MAX_EVENTS];

    while (g_running)
    {
        std::cout << "running" << std::endl;
        int n = epoll_wait(epfd, events, WEBSERV_EPOLL_MAX_EVENTS, WEBSERV_EPOLL_TIMEOUT);
        std::cout << "wakeup" << std::endl;
        if (n < 0)
        {
            if (errno == EINTR)
                continue; // interrupted by signal
            perror("epoll_wait failed");
            return FAILURE;
        }

        for (int i = 0; i < n; ++i)
        {
            EpollHandler *handler = static_cast<EpollHandler *>(events[i].data.ptr);
            if (handler->handle_event(events[i].events) != SUCCES)
            {
                return FAILURE;
            }
        }
    }

    return SUCCES;
}

} // namespace WebServ
