#include "classes/Listener.hpp"

namespace Listener
{

std::vector<Entry> listener;

Entry::Entry()
    : server_id(-1), port(0), fd(-1), initialized(false)
{
}

Entry::Entry(const Entry &other)
{
    *this = other;
}

Entry &Entry::operator=(const Entry &other)
{
    if (this != &other)
    {
        server_id = other.server_id;
        port = other.port;
        fd = other.fd;
        initialized = other.initialized;
    }
    return *this;
}

Entry::~Entry()
{
}

Entry::Entry(int server_id, in_port_t port)
    : server_id(server_id), port(port), fd(-1), initialized(false)
{
}

// Overrides
int Entry::get_fd() const
{
    return fd;
}

int Entry::handle_event(uint32_t events)
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

        int client_fd = accept(fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0)
        {
            // No more clients waiting
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }

            perror("accept");
            return FAILURE;
        }

        log::log(LISTENER, "Accepted client fd=" + std::to_string(client_fd));

        // Make client non-blocking
        int flags = fcntl(client_fd, F_GETFL, 0);
        if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            perror("fcntl client");
            close(client_fd);
            continue;
        }

        // For now, just close immediately (until Client class exists)
        close(client_fd);
    }

    return SUCCES;
}

int Entry::init()
{

#ifdef DEBUG
    if (is_initialized())
    {
        throw SetupError("Init Called Multiple times, for the same Listener::Entry!");
    }
#endif

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

    initialized = true;
    return SUCCES;
}

void Entry::quit()
{
    if (is_initialized())
    {
        close(fd);
        fd = -1;
        initialized = false;
    }
}

bool Entry::is_initialized() const
{
    return initialized;
}

std::string Entry::to_string() const
{
    return std::string("Listener(server_id=") + std::to_string(server_id) + ", port=" + std::to_string(port) + ", fd=" + std::to_string(fd) + ", initialized=" + (initialized ? "true" : "false") + ")";
}

std::ostream &operator<<(std::ostream &os, const Entry &e)
{
    return os << e.to_string();
}

int init()
{
    for (size_t i = 0; i < listener.size(); i++)
    {
        if (listener[i].init() != SUCCES)
        {
            return FAILURE;
        }
    }
    return SUCCES;
}

void quit()
{
    for (size_t i = 0; i < listener.size(); i++)
    {
        listener[i].quit();
    }
}

void display()
{
    log::log(DISPLAY, LISTENER);
    for (size_t i = 0; i < listener.size(); i++)
    {
        std::cout << i << ": " << listener[i] << std::endl;
    }
}

} // namespace Listener
