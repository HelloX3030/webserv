#pragma once

#include <cstdint>

namespace WebServ
{

extern int epfd;

}

class EpollHandler
{

  private:
  public:
    virtual ~EpollHandler()
    {
    }

    // Functions
    virtual int get_fd() const = 0;
    virtual void add_handler() = 0;
    virtual void handle_event(uint32_t events) = 0;
    virtual bool is_initialized() const = 0;
};
