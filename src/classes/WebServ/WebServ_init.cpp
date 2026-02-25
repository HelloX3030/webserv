#include "WebServ.hpp"

namespace WebServ
{

int init()
{

#ifdef DEBUG
    log::log(FUNCTION, "WebServ::init()");
    if (epfd != -1)
    {
        throw SetupError("Multiple Calls to WebServ::init()!");
    }
#endif

    // epoll Setup
    epfd = epoll_create1(0);
    if (epfd < 0)
    {
        perror("epoll_create1 failed");
        return FAILURE;
    }

    return SUCCES;
}

} // namespace WebServ
