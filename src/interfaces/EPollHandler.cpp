#include "interfaces/EPollHandler.hpp"

namespace WebServ
{

int epfd = -1;
std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;

} // namespace WebServ
