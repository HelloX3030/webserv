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

    // Getter
    const ServerConfig &get_config() const;

    // Public Functions
    std::string to_string() const;
};

namespace WebServ
{

extern const Server* default_server;
extern std::vector<Server> servers;
void add_server(const ServerConfig& config);

} // namespace WebServ
