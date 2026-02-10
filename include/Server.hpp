#pragma once

#include "base.hpp"

class Server
{
  private:
    // Server Config

    // Server Vars
    int server_fd;

    // Functions
    void start();
    void stop();

  public:
    Server();
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    // Special Constructors
    void parse(const std::string &file_path);
    void parse_args(int argc, char **argv);
    void run();
};
