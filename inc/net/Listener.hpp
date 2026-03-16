#pragma once

#include "base/Fd.hpp"
#include "config/Config.hpp"
#include "net/EPollHandler.hpp"
#include <unordered_map>

class Server;

class Listener final : public EpollHandler
{
  private:
    Server &default_server;
    std::unordered_map<std::string, Server *> host_to_server;
    Fd fd;

  public:
    Listener() = delete;
    Listener(const Listener &other) = delete;
    Listener &operator=(const Listener &other) = delete;
    ~Listener();

    // Custom Constructors
    Listener(ListenAddress listen_adress, Server &default_server);

    // Getter
    const ServerConfig &get_default_server() const;
    const ServerConfig &get_server_config(const std::string &host) const;

    // Setters
    void add_host(const std::string &new_host, Server &server);

    // Overrides
    int get_fd() const override;
    uint32_t get_events() const override;
    void handle_event(uint32_t events) override;
    bool should_close() const override;
    std::string to_string() const override;
};
std::ostream &operator<<(std::ostream &os, const Listener &e);

namespace WebServ
{

void add_listener(ListenAddress adress, const std::vector<std::string> &hosts, Server &server);

}
