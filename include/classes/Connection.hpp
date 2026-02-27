#pragma once

#include "base/base.hpp"
#include "interfaces/EPollHandler.hpp"

enum class ConnectionState
{
    READ,
    WRITE,
    CLOSE
};
std::string to_string(ConnectionState state);

class Connection : public EpollHandler
{
  private:
    Fd fd;
    ConnectionState state;
    std::string read_buffer;
    std::string write_buffer;

  public:
    Connection() = delete;
    Connection(const Connection &other) = delete;
    Connection &operator=(const Connection &other) = delete;
    ~Connection();

    // Special Constructors
    Connection(int fd);

    // Overrides
    int get_fd() const override;
    uint32_t get_events() const override;
    void handle_event(uint32_t events) override;
    bool should_close() const override;
    std::string to_string() const override;
};
std::ostream &operator<<(std::ostream &os, const Connection &connection);

namespace WebServ
{

void add_connection(int fd);

}
