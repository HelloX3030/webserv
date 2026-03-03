#pragma once

#include "base/base.hpp"

#include "classes/Listener.hpp"

class Server
{
  private:
    // Server Config

    // Server Vars

    // Functions

  public:
    Server();
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    // Public Functions
    void parse(const std::string &file_path);
};
