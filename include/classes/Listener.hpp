#pragma once

#include "base/base.hpp"
#include "interfaces/EPollHandler.hpp"

namespace Listener
{

class Entry : public EpollHandler
{
  private:
    int server_id;
    in_port_t port;
    int fd;
    bool initialized;

  public:
    Entry();
    Entry(const Entry &other);
    Entry &operator=(const Entry &other);
    ~Entry();

    // Custom Constructors
    Entry(int server_id, in_port_t port);

    // Overrides
    int get_fd() const override;
    void add_handler() override;
    int handle_event(uint32_t events) override;
    bool is_initialized() const override;

    // Functions
    int init();
    void quit();
    std::string to_string() const;
};

std::ostream &operator<<(std::ostream &os, const Entry &e);

extern std::vector<Entry> listener;

int init();
void quit();
void display();

} // namespace Listener
