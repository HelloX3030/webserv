#include "classes/Listener.hpp"
#include "classes/Server.hpp"

Listener::Listener()
    : fd(-1), server(nullptr)
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
        server = other.server;
    }
    return *this;
}

Listener::~Listener()
{
}
