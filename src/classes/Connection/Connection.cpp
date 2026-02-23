#include "classes/Connection.hpp"

namespace Connection
{

Connection::Connection()
    : server_id(-1), fd(-1), state(ConnectionState::READING)
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

Connection::Connection(int server_id, int fd)
{
    this->server_id = server_id;
    this->fd = fd;
}

// Overrides
int Connection::get_fd() const
{
    return fd;
}

int Connection::handle_event(uint32_t events)
{
    (void)events;
    std::cout << "Connection event" << std::endl;
    return SUCCES;
}

bool Connection::is_initialized() const
{
    return server_id != -1 && fd >= 0;
}

void Connection::quit()
{
    if (is_initialized())
    {
        close(fd);
        fd = -1;
    }
}

} // namespace Connection
