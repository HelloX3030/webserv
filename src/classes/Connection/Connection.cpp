#include "classes/Connection.hpp"

Connection::Connection()
    : fd(-1), state(ConnectionState::READ)
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

Connection::Connection(int fd)
    : fd(fd), state(ConnectionState::READ)
{
}

// Overrides
int Connection::get_fd() const
{
    return fd;
}

void Connection::handle_event(uint32_t events)
{
    (void)events;
    std::cout << "Connection event" << std::endl;
}

void Connection::quit()
{
    close(fd);
    fd = -1;
}

std::string Connection::to_string() const
{
    return "Connection(fd=" + std::to_string(fd) + ", state=" + ::to_string(state) + ", read_buffer_size=" + std::to_string(read_buffer.size()) + ", write_buffer_size=" + std::to_string(write_buffer.size()) + ")";
}

std::ostream &operator<<(std::ostream &os, const Connection &connection)
{
    return os << connection.to_string();
}

std::string to_string(ConnectionState state)
{
    switch (state)
    {
    case ConnectionState::READ:
        return READ;
    case ConnectionState::WRITE:
        return WRITE;
    case ConnectionState::CLOSE:
        return CLOSE;
    default:
        return UNKNOWN;
    }
}
