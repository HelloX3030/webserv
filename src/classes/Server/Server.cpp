#include "classes/Server.hpp"

Server::Server()
{
}

Server::Server(const Server &other)
{
    *this = other;
}

Server &Server::operator=(const Server &other)
{
    if (this != &other)
    {
        listener = other.listener;
    }
    return *this;
}

Server::~Server()
{
}
