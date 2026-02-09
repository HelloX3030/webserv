#pragma once

#include <iostream>
#include <stdexcept>

class WebServ
{
  private:
    // Settings

  public:
    WebServ();
    WebServ(const WebServ &other);
    WebServ &operator=(const WebServ &other);
    ~WebServ();

    // Funcs
    void parse(int argc, char **argv);
};
