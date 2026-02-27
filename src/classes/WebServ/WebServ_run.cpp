#include "WebServ.hpp"

namespace WebServ
{

void run()
{
    struct epoll_event events[EPOLL_MAX_EVENTS];

    while (g_running)
    {
        int n = epoll_wait(epfd, events, ::WebServ::EPOLL_MAX_EVENTS, ::WebServ::EPOLL_TIMEOUT);

#ifdef DEBUG
        std::cout << BR << std::endl;
        log::log(WEB_SERV, "e_poll wakeup...");
        display();
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
#ifdef DEBUG
            log::log(WEB_SERV, handler->get_fd(), handler->to_string());
#endif
            handler->handle_event(events[i].events);

            // Close when needed
            if (handler->should_close())
            {
                remove_epoll_handler(handler->get_fd());
            }
            else
            {
                // Update Events
                handler->update_epoll_events();
            }
        }
    }
}

} // namespace WebServ
