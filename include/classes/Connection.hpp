#pragma once

#include "base/base.hpp"
#include "interfaces/EPollHandler.hpp"

namespace Connection
{

enum class ConnectionState
{
    READING,
    WRITING,
    CLOSED
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
};

extern std::vector<Connection> connections;
void quit();

} // namespace Connection
