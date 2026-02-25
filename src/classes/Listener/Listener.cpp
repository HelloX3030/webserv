#include "classes/Listener.hpp"
#include "classes/Connection.hpp"

Listener::Listener()
    : port(0), fd(-1)
{
}

Listener::Listener(const Listener &other)
{
    *this = other;
}

Listener &Listener::operator=(const Listener &other)
{
    if (this != &other)
    {
        port = other.port;
        fd = other.fd;
    }
    return *this;
}

Listener::~Listener()
{
}

Listener::Listener(in_port_t port)
    : port(port), fd(-1)
{
}

// Overrides
int Listener::get_fd() const
{
    return fd;
}

int Listener::handle_event(uint32_t events)
{
    if (events & (EPOLLERR | EPOLLHUP))
    {
        log::log(LISTENER, std::string("EPOLLERR or EPOLLHUP on fd=") + std::to_string(fd), log::LogType::ERROR);
        return FAILURE;
    }

    if (!(events & EPOLLIN))
        return SUCCES;

#ifdef DEBUG
    std::cout << "Listener ready on port " << port << std::endl;
#endif

    // Accept all waiting Clients
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int connection_fd = accept(fd, (struct sockaddr *)&client_addr, &len);
        if (connection_fd < 0)
        {
            // No more clients waiting
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }

            perror("accept");
            return FAILURE;
        }

        log::log(LISTENER, "Accepted client fd=" + std::to_string(connection_fd));

        // Make client non-blocking
        int flags = fcntl(connection_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            perror("fcntl client");
            close(connection_fd);
            continue;
        }

        // Register to e_poll
        struct epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.ptr = this;

        if (epoll_ctl(WebServ::epfd, EPOLL_CTL_ADD, connection_fd, &ev) < 0)
        {
            perror("epoll_ctl ADD connection failed");
            close(connection_fd);
            return FAILURE;
        }

        // Add New Connection
        // TODO
    }

    return SUCCES;
}

int Listener::init()
{

    // Create Socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        perror("socket");
        fd = -1;
        return FAILURE;
    }

    // make socket non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl(F_GETFL)");
        close(fd);
        fd = -1;
        return FAILURE;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl(F_SETFL)");
        close(fd);
        fd = -1;
        return FAILURE;
    }

    // Reuse Adress
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(fd);
        fd = -1;
        return FAILURE;
    }

    // Bind to Port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;         // IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0 (all interfaces)
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(fd);
        fd = -1;
        return FAILURE;
    }

    if (listen(fd, SOMAXCONN) < 0)
    {
        perror("listen");
        close(fd);
        fd = -1;
        return FAILURE;
    }

// Add To E-Poll Queue
#ifdef DEBUG
    if (WebServ::epfd == -1)
        throw SetupError("Add Handler Called before WebServ::init()");
#endif

    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));

    ev.events = EPOLLIN; // start simple (level-triggered)
    ev.data.ptr = static_cast<void *>(this);

    if (epoll_ctl(WebServ::epfd, EPOLL_CTL_ADD, this->get_fd(), &ev) < 0)
    {
        throw std::runtime_error("epoll_ctl ADD failed");
    }

    return SUCCES;
}

void Listener::quit()
{
    close(fd);
    fd = -1;
}

std::string Listener::to_string() const
{
    return "Listener(port=" + std::to_string(port) + ", fd=" + std::to_string(fd) + ")";
}

std::ostream &operator<<(std::ostream &os, const Listener &e)
{
    return os << e.to_string();
}
