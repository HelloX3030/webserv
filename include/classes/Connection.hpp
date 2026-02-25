#pragma once

#include "base/base.hpp"
#include "interfaces/EPollHandler.hpp"

enum class ConnectionState
{
    READ,
    WRITE,
    CLOSE
};

class Connection : public EpollHandler
{
  private:
    int fd;
    ConnectionState state;
    std::string read_buffer;
    std::string write_buffer;

  public:
    Connection();
    Connection(const Connection &other);
    Connection &operator=(const Connection &other);
    ~Connection();

    // Special Constructors
    Connection(int fd);

    // Overrides
    int get_fd() const override;
    int handle_event(uint32_t events) override;

    // Functions
    void quit();
    std::string to_string() const;
};
std::ostream &operator<<(std::ostream &os, const Connection &connection);

std::string to_string(ConnectionState state);
