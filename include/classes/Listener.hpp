#pragma once

#include "base/base.hpp"

namespace Listener
{

class Entry
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

    // Functions
    int init();
    void quit();
    bool is_initialized() const;
};

extern std::vector<Entry> listener;

int init();
void quit();

} // namespace Listener
