#pragma once

#include "base/base.hpp"

class EpollHandler;

namespace WebServ
{

extern int epfd;
extern std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;

} // namespace WebServ

class EpollHandler
{

  private:
  public:
    virtual ~EpollHandler()
    {
    }

    // Functions
    virtual int get_fd() const = 0;
    virtual int handle_event(uint32_t events) = 0;
    virtual bool is_initialized() const = 0;
};
