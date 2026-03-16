#pragma once

#include "config/Config.hpp"
#include "net/Listener.hpp"

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

    // Getter
    const ServerConfig &get_config() const;

    // Public Functions
    std::string to_string() const;
};

namespace WebServ
{

extern std::vector<Server> servers;
void add_server(const ServerConfig &config);

} // namespace WebServ
