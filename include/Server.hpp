#pragma once

#include "ServerConfig.hpp"

class Server
{
  private:
    ServerConfig config;

  public:
    Server();
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    // Funcs
};
