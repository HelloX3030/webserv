#include "classes/Listener.hpp"
#include "classes/Server.hpp"

namespace Listener
{

std::vector<Entry> listener;

Entry::Entry()
    : server_id(-1), port(-1), fd(-1), valid(false)
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
        valid = other.valid;
    }
    return *this;
}

Entry::~Entry()
{
}

Entry::Entry(int server_id, int port)
    : server_id(server_id), port(port), fd(-1), valid(false)
{
}

int Entry::init()
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

    valid = true;
    return SUCCES;
}

void Entry::quit()
{
    if (get_valid())
    {
        close(fd);
        fd = -1;
        valid = false;
    }
}

bool Entry::get_valid() const
{
    return valid;
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

} // namespace Listener
