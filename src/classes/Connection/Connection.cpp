#include "classes/Connection.hpp"

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
    return fd.get();
}

void Connection::handle_event(uint32_t events)
{
    (void)events;
    std::cout << "Connection event" << std::endl;
}

std::string Connection::to_string() const
{
    return "Connection(fd=" + std::to_string(get_fd()) + ", state=" + ::to_string(state) + ", read_buffer_size=" + std::to_string(read_buffer.size()) + ", write_buffer_size=" + std::to_string(write_buffer.size()) + ")";
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

namespace WebServ
{

void add_connection(int fd)
{
    auto new_connection = std::make_unique<Connection>(fd);
    add_epoll_handler(std::move(new_connection));
}

} // namespace WebServ
