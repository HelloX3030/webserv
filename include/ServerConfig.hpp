#pragma once

#include "base.hpp"

class ServerConfig
{
  private:
  public:
    ServerConfig();
    ServerConfig(const ServerConfig &other);
    ServerConfig &operator=(const ServerConfig &other);
    ~ServerConfig();

    // Funcs
    void parse(const std::string &file_path);
    void parse_args(int argc, char **argv);
};
