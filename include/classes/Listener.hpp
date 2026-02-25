#pragma once

#include "base/base.hpp"
#include "interfaces/EPollHandler.hpp"

class Listener : public EpollHandler
{
  private:
    in_port_t port;
    int fd;

  public:
    Listener() = delete;
    Listener(const Listener &other) = delete;
    Listener &operator=(const Listener &other) = delete;
    ~Listener();

    // Custom Constructors
    Listener(in_port_t port);

    // Overrides
    int get_fd() const override;
    int handle_event(uint32_t events) override;

    // Functions
    std::string to_string() const;
};
std::ostream &operator<<(std::ostream &os, const Listener &e);
