#include "WebServ.hpp"
#include "base/defines.hpp"
#include "base/errors.hpp"
#include "base/logging.hpp"
#include "core/signal.hpp"

#include <chrono>
#include <cerrno>
#include <sys/wait.h>
#include <sys/epoll.h>

namespace
{

void reap_terminated_children()
{
    int status = 0;
    while (true)
    {
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0)
            continue;
        if (pid == 0)
            return;
        if (pid == -1 && errno == EINTR)
            continue;
        return;
    }
}

void reap_timed_out_connections()
{
    const auto now = std::chrono::steady_clock::now();

    std::vector<int> expired_fds;
    expired_fds.reserve(WebServ::epoll_handlers.size());

    for (std::size_t fd = 0; fd < WebServ::epoll_handlers.size(); ++fd)
    {
        EpollHandler *handler = WebServ::epoll_handlers[fd].get();
        if (handler == nullptr)
            continue;

        Connection *connection = dynamic_cast<Connection *>(handler);
        if (connection != nullptr && connection->has_timed_out(now))
            expired_fds.push_back(static_cast<int>(fd));
    }

    for (int fd : expired_fds)
        WebServ::remove_epoll_handler(fd);
}

} // namespace

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
        logging::log(WEB_SERV, "e_poll wakeup...");
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
            logging::log(WEB_SERV, handler->get_fd(), handler->to_string());
#endif
            handler->handle_event(events[i].events);

            // Close when needed
            if (handler->should_close())
            {
                remove_epoll_handler(handler->get_fd());
            }
        }

        if (g_sigchld_pending)
        {
            g_sigchld_pending = 0;
            reap_terminated_children();
        }

        reap_timed_out_connections();
    }
}

} // namespace WebServ
