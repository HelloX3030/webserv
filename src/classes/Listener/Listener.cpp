#include "classes/Listener.hpp"
#include "classes/Connection.hpp"

Listener::~Listener()
{
}

Listener::Listener(in_port_t port)
    : port(port)
{
    // Create Socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        throw std::system_error(errno, std::generic_category(), "socket");
    }

    // make socket non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        throw std::system_error(errno, std::generic_category(), "fcntl(F_GETFL)");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        throw std::system_error(errno, std::generic_category(), "fcntl(F_SETFL)");
    }

    // Reuse Adress
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "setsockopt");
    }

    // Bind to Port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;         // IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0 (all interfaces)
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "bind");
    }

    if (listen(fd, SOMAXCONN) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "listen");
    }

    this->fd.set(fd);
}

// Overrides
int Listener::get_fd() const
{
    return fd.get();
}

void Listener::handle_event(uint32_t events)
{
    if (events & (EPOLLERR | EPOLLHUP))
    {
        // TODO => I guess close connection?
    }

    if (!(events & EPOLLIN))
        return;

#ifdef DEBUG
    std::cout << "Listener ready on port " << port << std::endl;
#endif

    // Accept all waiting Clients
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int connection_fd = accept(fd.get(), (struct sockaddr *)&client_addr, &len);
        if (connection_fd < 0)
        {
            // No more clients waiting
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }

            throw std::system_error(errno, std::generic_category(), "accept");
        }

        log::log(LISTENER, "Accepted client fd=" + std::to_string(connection_fd));

        // Make client non-blocking
        int flags = fcntl(connection_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            close(connection_fd);
            throw std::system_error(errno, std::generic_category(), "fcntl");
        }

        // Register to e_poll
        // TODO: Use add_epoll_handler instead!
        struct epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = this;

        if (epoll_ctl(WebServ::epfd, EPOLL_CTL_ADD, connection_fd, &ev) < 0)
        {
            close(connection_fd);
            throw std::system_error(errno, std::generic_category(), "epoll_ctl ADD connection failed");
        }

        // Add New Connection
        // TODO
    }
}

std::string Listener::to_string() const
{
    return "Listener(port=" + std::to_string(port) + ", fd=" + std::to_string(fd.get()) + ")";
}

std::ostream &operator<<(std::ostream &os, const Listener &e)
{
    return os << e.to_string();
}

namespace WebServ
{

void add_listener(in_port_t port)
{
    auto new_listener = std::make_unique<Listener>(port);
    add_epoll_handler(std::move(new_listener));
}

} // namespace WebServ
