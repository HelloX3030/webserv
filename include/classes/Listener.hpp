#pragma once

#include "base/base.hpp"

class Server;

class Listener
{
  private:
    int fd;
    Server *server;

  public:
    Listener();
    Listener(const Listener &other);
    Listener &operator=(const Listener &other);
    ~Listener();
};
