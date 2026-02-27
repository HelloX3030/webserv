#include "interfaces/EPollHandler.hpp"

EpollHandler::~EpollHandler()
{
}

void EpollHandler::update_epoll_events()
{
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));

    ev.events = get_events();
    ev.data.ptr = static_cast<void *>(this);

    if (epoll_ctl(WebServ::epfd, EPOLL_CTL_MOD, get_fd(), &ev) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "epoll_ctl");
    }
}

namespace WebServ
{

int epfd = -1;
std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;

void add_epoll_handler(std::unique_ptr<EpollHandler> new_epoll_handler)
{
#ifdef DEBUG
    if (WebServ::epfd == -1)
        throw SetupError("Add Listener Called before WebServ::init()");
#endif

    // Add to E-Poll Queue
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));

    ev.events = new_epoll_handler->get_events();
    ev.data.ptr = static_cast<void *>(new_epoll_handler.get());

    int fd = new_epoll_handler->get_fd();
    if (epoll_ctl(WebServ::epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "epoll_ctl");
    }

    // Add to Handlers
    if (fd < 0)
    {
#ifdef DEBUG
        throw SetupError("Tried to add Invalid fd to e_poll Queue");
#endif
        return;
    }

    if (UNLIKELY(static_cast<std::size_t>(fd) >= epoll_handlers.size()))
    {
        epoll_handlers.resize(fd + EPOLL_HANDLERS_BATCH_SIZE);
    }
    epoll_handlers[fd] = std::move(new_epoll_handler);
}

void remove_epoll_handler(int fd)
{
    if (fd < 0)
    {
#ifdef DEBUG
        throw SetupError("Tried to remove negative fd EPollHandler");
#endif
        return;
    }
    if (static_cast<std::size_t>(fd) >= epoll_handlers.size())
    {
#ifdef DEBUG
        throw SetupError("Tried to remove EPollHandler at to large fd");
#endif
        return;
    }
    if (!epoll_handlers[fd])
    {
#ifdef DEBUG
        throw SetupError("Tried to remove invalid EPollHandler");
#endif
        return;
    }

    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    epoll_handlers[fd].reset();
}

} // namespace WebServ
