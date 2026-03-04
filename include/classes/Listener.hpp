#pragma once

#include "base/base.hpp"
#include "classes/Config.hpp"
#include "interfaces/EPollHandler.hpp"

class Listener final : public EpollHandler
{
  private:
    Fd fd;

  public:
    Listener() = delete;
    Listener(const Listener &other) = delete;
    Listener &operator=(const Listener &other) = delete;
    ~Listener();

    // Custom Constructors
    Listener(ListenAddress adress);

    // Overrides
    int get_fd() const override;
    uint32_t get_events() const override;
    void handle_event(uint32_t events) override;
    bool should_close() const override;
    std::string to_string() const override;
};
std::ostream &operator<<(std::ostream &os, const Listener &e);

namespace WebServ
{

void add_listener(ListenAddress adress);

}
