#pragma once

#include "base/base.hpp"

#include "classes/Listener.hpp"

class Server
{
  private:
    // Server Config

    // Server Vars
    std::vector<Listener> listener;

    // Functions
    void start();
    void respond();
    void stop();

  public:
    Server();
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    // Public Functions
    void parse(const std::string &file_path);
    void run();
};
