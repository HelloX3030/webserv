#pragma once

#include "Server.hpp"
#include "base.hpp"

class WebServ
{
  private:
    std::vector<Server> servers;

  public:
    WebServ();
    WebServ(const WebServ &other);
    WebServ &operator=(const WebServ &other);
    ~WebServ();

    // Functions
    void parse(int argc, char **argv);
    void start();
};
