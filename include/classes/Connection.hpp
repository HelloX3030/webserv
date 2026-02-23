#pragma once

#include "base/base.hpp"
#include "interfaces/EPollHandler.hpp"

namespace Connection
{

enum class ConnectionState
{
    READ,
    WRITE,
    CLOSE
};

class Connection : public EpollHandler
{
  private:
    int server_id;
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
    Connection(int server_id, int fd);

    // Overrides
    int get_fd() const override;
    int handle_event(uint32_t events) override;
    bool is_initialized() const override;

    // Functions
    void quit();
    std::string to_string() const;
};
std::ostream &operator<<(std::ostream &os, const Connection &connection);

extern std::vector<Connection> connections;
void quit();
void display();

} // namespace Connection

std::string to_string(Connection::ConnectionState state);
