#include "classes/Connection.hpp"

Connection::Connection()
{
}

Connection::Connection(const Connection &other)
{
    *this = other;
}

Connection &Connection::operator=(const Connection &other)
{
    if (this != &other)
    {
        server_id = other.server_id;
        fd = other.fd;
        state = other.state;
        read_buffer = other.read_buffer;
        write_buffer = other.write_buffer;
    }
    return *this;
}

Connection::~Connection()
{
}
