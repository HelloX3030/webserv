#include "net/Listener.hpp"
#include "base/defines.hpp"
#include "base/errors.hpp"
#include "base/logging.hpp"
#include "config/Config.hpp"
#include "core/Server.hpp"
#include "net/Connection.hpp"
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{

std::unordered_map<ListenAddress, Listener *> adress_to_listener;

}

Listener::~Listener()
{
}

Listener::Listener(ListenAddress listen_adress, Server &default_server)
    : default_server(default_server)
{
    // Create Socket
    fd.set(socket(AF_INET, SOCK_STREAM, 0));
    if (fd.get() < 0)
        throw std::system_error(errno, std::generic_category(), "socket");

    // make socket non-blocking
    int flags = fcntl(fd.get(), F_GETFL, 0);
    if (flags == -1)
        throw std::system_error(errno, std::generic_category(), "fcntl(F_GETFL)");

    if (fcntl(fd.get(), F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::system_error(errno, std::generic_category(), "fcntl(F_SETFL)");

    // Reuse Address
    int opt = 1;
    if (setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::system_error(errno, std::generic_category(), "setsockopt");

    // Resolve address
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // suitable for bind()

    int ret = getaddrinfo(listen_adress.host.c_str(), NULL, &hints, &result);
    if (ret != 0)
        throw std::runtime_error(gai_strerror(ret));

    // Extract sockaddr_in and set port
    struct sockaddr_in addr = *(struct sockaddr_in *)result->ai_addr;
    addr.sin_port = htons(listen_adress.port);

    // Bind
    if (bind(fd.get(), (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        freeaddrinfo(result);
        throw std::system_error(errno, std::generic_category(), "bind");
    }

    freeaddrinfo(result);

    // Listen
    if (listen(fd.get(), SOMAXCONN) < 0)
        throw std::system_error(errno, std::generic_category(), "listen");
}

const ServerConfig &Listener::get_default_server() const
{
    return default_server.get_config();
}

const ServerConfig &Listener::get_server_config(const std::string &host) const
{
    auto it = host_to_server.find(host);

    if (it == host_to_server.end())
    {
        return default_server.get_config();
    }

    return it->second->get_config();
}

void Listener::add_host(const std::string &new_host, Server &server)
{
    auto it = host_to_server.find(new_host);
    if (it != host_to_server.end())
    {
        logging::log(WARNING, "Duplicated server_name");
    }
    else
    {
        host_to_server[new_host] = &server;
    }
}

// Overrides
int Listener::get_fd() const
{
    return fd.get();
}

uint32_t Listener::get_events() const
{
    return EPOLLIN;
}

void Listener::handle_event(uint32_t events)
{
    // epoll reported socket error
    if (events & (EPOLLERR | EPOLLHUP))
    {
        logging::log(LISTENER, "EPOLLERR", logging::LogType::ERROR);
    }

    // listener only cares about readable events
    if (!(events & EPOLLIN))
        return;

    // drain accept queue
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int connection_fd = accept(fd.get(), (struct sockaddr *)&client_addr, &len);
        if (connection_fd < 0)
        {
            // accept queue empty
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            // syscall interrupted
            if (errno == EINTR)
                continue;

            // client aborted before accept
            if (errno == ECONNABORTED)
                continue;

            // fd limit reached
            if (errno == EMFILE || errno == ENFILE)
            {
                logging::log(LISTENER, "FD limit reached", logging::LogType::ERROR);
                break;
            }

            // unexpected accept error
            int err = errno;
            logging::log(LISTENER, "accept failed: " + std::string(strerror(err)), logging::LogType::ERROR);
            continue;
        }

        // new client connected
        logging::log(LISTENER, "Accepted client fd=" + std::to_string(connection_fd));

        // set client socket non-blocking
        int flags = fcntl(connection_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            int err = errno;
            logging::log(LISTENER, "fcntl failed: " + std::string(strerror(err)), logging::LogType::ERROR);
            close(connection_fd);
            continue;
        }

        // register connection with server
        WebServ::add_connection(*this, connection_fd);
    }
}

bool Listener::should_close() const
{
    return false;
}

std::string Listener::to_string() const
{
    std::string result = "Listener(fd=" + std::to_string(fd.get()) + ", hosts=[";

    bool first = true;
    for (const auto &[host, _] : host_to_server)
    {
        if (!first)
            result += ", ";
        result += host;
        first = false;
    }

    result += "])";
    return result;
}

std::ostream &operator<<(std::ostream &os, const Listener &e)
{
    return os << e.to_string();
}

namespace WebServ
{

void add_listener(ListenAddress adress, const std::vector<std::string> &hosts, Server &server)
{
    Listener *listener;
    auto it = adress_to_listener.find(adress);
    if (it == adress_to_listener.end())
    {
        auto new_listener = std::make_unique<Listener>(adress, server);
        listener = new_listener.get();
        adress_to_listener[adress] = listener;
        add_epoll_handler(std::move(new_listener));
    }
    else
    {
        listener = it->second;
    }

#ifdef DEBUG
    if (!listener)
    {
        throw SetupError("Listener is nullptr");
    }
#endif

    // Add Hosts to Map
    for (auto &host : hosts)
    {
        listener->add_host(host, server);
    }
}

} // namespace WebServ
