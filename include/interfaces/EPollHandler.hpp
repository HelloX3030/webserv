#pragma once

#include "base/base.hpp"

class EpollHandler
{

  private:
  public:
    virtual ~EpollHandler()
    {
    }

    // Functions
    virtual int get_fd() const = 0;
    virtual void handle_event(uint32_t events) = 0;
    virtual std::string to_string() const = 0;
};

namespace WebServ
{

extern int epfd;
extern std::vector<std::unique_ptr<EpollHandler>> epoll_handlers;

void add_epoll_handler(std::unique_ptr<EpollHandler> new_epoll_handler);

} // namespace WebServ
