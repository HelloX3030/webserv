#include "classes/Listener.hpp"
#include "classes/Server.hpp"

Listener::Listener()
    : fd(-1), server_id(-1)
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
        fd = other.fd;
        server_id = other.server_id;
    }
    return *this;
}

Listener::~Listener()
{
}
