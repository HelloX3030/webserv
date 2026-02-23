#include "classes/Connection.hpp"

namespace Connection
{

std::vector<Connection> connections;

Connection::Connection()
    : server_id(-1), fd(-1), state(ConnectionState::READ)
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

std::string Connection::to_string() const
{
    return std::string("Connection(server_id=") + std::to_string(server_id) + ", fd=" + std::to_string(fd) + ", state=" + ::to_string(state) + ", read_buffer_size=" + std::to_string(read_buffer.size()) + ", write_buffer_size=" + std::to_string(write_buffer.size()) + ")";
}

std::ostream &operator<<(std::ostream &os, const Connection &connection)
{
    return os << connection.to_string();
}

void quit()
{
    for (size_t i = 0; i < connections.size(); i++)
    {
        connections[i].quit();
        connections.clear();
    }
}

void display()
{
    log::log(DISPLAY, CONNECTION);

    // No Connections
    if (connections.size() == 0)
    {
        std::cout << ELLIPSIS << std::endl;
    }

    // Print Elements
    for (size_t i = 0; i < connections.size(); i++)
    {
        log::log(CONNECTION, i, connections[i].to_string());
    }
}

} // namespace Connection

std::string to_string(Connection::ConnectionState state)
{
    switch (state)
    {
    case Connection::ConnectionState::READ:
        return READ;
    case Connection::ConnectionState::WRITE:
        return WRITE;
    case Connection::ConnectionState::CLOSE:
        return CLOSE;
    default:
        return UNKNOWN;
    }
}
