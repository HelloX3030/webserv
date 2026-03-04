#pragma once

#include "base/base.hpp"

#include "classes/Config.hpp"
#include "classes/Listener.hpp"

class Server final
{
  private:
    // Server Config
    ServerConfig config;

    // Server Vars

    // Functions

  public:
    Server() = delete;
    Server(const Server &other) = default;
    Server &operator=(const Server &other) = default;
    ~Server() = default;

    // Special Constructor
    Server(ServerConfig config);

    // Public Functions
    void parse(const std::string &file_path);
    std::string to_string() const;
};

namespace WebServ
{

extern std::vector<Server> servers;

} // namespace WebServ
