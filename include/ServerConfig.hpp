#pragma once

#include "base.hpp"

class ServerConfig
{
  private:
  public:
    ServerConfig(std::string file_path);
    ServerConfig(const ServerConfig &other);
    ServerConfig &operator=(const ServerConfig &other);
    ~ServerConfig();
};
