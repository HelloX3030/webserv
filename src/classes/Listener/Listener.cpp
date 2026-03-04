#include "classes/Listener.hpp"
#include "classes/Connection.hpp"

Listener::~Listener()
{
}

Listener::Listener(ListenAddress address)
{
    // Create Socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::system_error(errno, std::generic_category(), "socket");

    // make socket non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::system_error(errno, std::generic_category(), "fcntl(F_GETFL)");

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::system_error(errno, std::generic_category(), "fcntl(F_SETFL)");

    // Reuse Address
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::system_error(errno, std::generic_category(), "setsockopt");

    // Resolve address
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // suitable for bind()

    int ret = getaddrinfo(address.host.c_str(), NULL, &hints, &result);
    if (ret != 0)
        throw std::runtime_error(gai_strerror(ret));

    // Extract sockaddr_in and set port
    struct sockaddr_in addr = *(struct sockaddr_in *)result->ai_addr;
    addr.sin_port = htons(address.port);

    // Bind
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        freeaddrinfo(result);
        throw std::system_error(errno, std::generic_category(), "bind");
    }

    freeaddrinfo(result);

    // Listen
    if (listen(fd, SOMAXCONN) < 0)
        throw std::system_error(errno, std::generic_category(), "listen");

    this->fd.set(fd);
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
    if (events & (EPOLLERR | EPOLLHUP))
    {
        // TODO => I guess close connection?
    }

    if (!(events & EPOLLIN))
        return;

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

        WebServ::add_connection(connection_fd);
    }
}

bool Listener::should_close() const
{
    return false;
}

std::string Listener::to_string() const
{
    return "Listener(fd=" + std::to_string(fd.get()) + ")";
}

std::ostream &operator<<(std::ostream &os, const Listener &e)
{
    return os << e.to_string();
}

namespace WebServ
{

void add_listener(ListenAddress adress)
{
    auto new_listener = std::make_unique<Listener>(adress);
    add_epoll_handler(std::move(new_listener));
}

} // namespace WebServ
