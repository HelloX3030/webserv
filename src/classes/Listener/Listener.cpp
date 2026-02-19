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

void Entry::add_handler()
{

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
}

void Entry::handle_event(uint32_t events)
{
    (void)events;
#ifdef DEBUG
    std::cout << "Listener::Entry::handle_event(): " << *this << std::endl;
#endif
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
