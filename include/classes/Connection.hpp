#pragma once

#include "base/base.hpp"

enum class ConnectionState
{
    READING,
    WRITING,
    CLOSED
};

class Connection
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
};
