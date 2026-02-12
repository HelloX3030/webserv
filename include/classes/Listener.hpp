#pragma once

#include "base/base.hpp"

class Server;

class Listener
{
  private:
    int fd;
    int server_id;

  public:
    Listener();
    Listener(const Listener &other);
    Listener &operator=(const Listener &other);
    ~Listener();
};
