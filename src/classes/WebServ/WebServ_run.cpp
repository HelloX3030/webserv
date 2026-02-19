#include "WebServ.hpp"

namespace WebServ
{

void run()
{
    struct epoll_event events[WEBSERV_EPOLL_MAX_EVENTS];

    while (g_running)
    {
        int n = epoll_wait(epfd, events, WEBSERV_EPOLL_MAX_EVENTS, WEBSERV_EPOLL_TIMEOUT);
        if (n < 0)
        {
            if (errno == EINTR)
                continue; // interrupted by signal
            throw std::runtime_error("epoll_wait failed");
        }

        for (int i = 0; i < n; ++i)
        {
            EpollHandler *handler = static_cast<EpollHandler *>(events[i].data.ptr);
            handler->handle_event(events[i].events);
        }
    }
}

} // namespace WebServ
