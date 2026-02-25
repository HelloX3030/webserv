#include "WebServ.hpp"

namespace WebServ
{

void run()
{
    struct epoll_event events[EPOLL_MAX_EVENTS];

    while (g_running)
    {
        int n = epoll_wait(epfd, events, EPOLL_MAX_EVENTS, EPOLL_MAX_EVENTS);
#ifdef DEBUG
        std::cout << BR << std::endl;
        log::log(WEB_SERV, "e_poll wakeup...");
#endif
        if (n < 0)
        {
            if (errno == EINTR)
                continue; // interrupted by signal
            throw std::system_error(errno, std::generic_category(), "epoll_wait");
        }

        for (int i = 0; i < n; ++i)
        {
            EpollHandler *handler = static_cast<EpollHandler *>(events[i].data.ptr);
            handler->handle_event(events[i].events);
        }
    }
}

} // namespace WebServ
