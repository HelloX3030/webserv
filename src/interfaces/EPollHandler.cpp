#include "interfaces/EPollHandler.hpp"

namespace WebServ
{

int epfd = -1;
std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;

void add_epoll_handler(std::unique_ptr<EpollHandler> new_epoll_handler)
{
    // Add to E-Poll Queue
#ifdef DEBUG
    if (WebServ::epfd == -1)
        throw SetupError("Add Listener Called before WebServ::init()");
#endif

    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));

    ev.events = EPOLLIN; // start simple (level-triggered) TODO: static get_flags function, that needs to be overritden, that gives the flags
    ev.data.ptr = static_cast<void *>(new_epoll_handler.get());

    if (epoll_ctl(WebServ::epfd, EPOLL_CTL_ADD, new_epoll_handler->get_fd(), &ev) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "epoll_ctl");
    }

    // Add to Handlers
    if (new_epoll_handler->get_fd() < 0)
    {
#ifdef DEBUG
        throw SetupError("Tried to add Invalid fd to e_poll Queue");
#endif
        return;
    }

    if (UNLIKELY(static_cast<std::size_t>(new_epoll_handler->get_fd()) >= epoll_handlers.size()))
    {
        epoll_handlers.resize(epoll_handlers.size() + EPOLL_HANDLERS_BATCH_SIZE);
    }
    epoll_handlers[new_epoll_handler->get_fd()] = std::move(new_epoll_handler);
}

} // namespace WebServ
