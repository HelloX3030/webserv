#include "WebServ.hpp"

namespace WebServ
{

void quit()
{
    // close epfd
    if (epfd != -1)
    {
        close(epfd);
        epfd = -1;
    }
}

} // namespace WebServ
