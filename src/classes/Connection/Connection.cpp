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

uint32_t Connection::get_events() const
{
    if (state == ConnectionState::CLOSE)
        return 0;

    uint32_t events = EPOLLIN;

    if (!write_buffer.empty())
        events |= EPOLLOUT;

    return events;
}

void Connection::handle_event(uint32_t events)
{
    // ---- Error handling ----
    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        state = ConnectionState::CLOSE;
        return;
    }

    // =========================
    // READ
    // =========================
    if (events & EPOLLIN)
    {
        while (true)
        {
            char buffer[4096];

            ssize_t n = ::read(fd.get(), buffer, sizeof(buffer));

            if (n > 0)
            {
                read_buffer.append(buffer, n);
            }
            else if (n == 0)
            {
                // peer closed
                state = ConnectionState::CLOSE;
                return;
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

                state = ConnectionState::CLOSE;
                return;
            }
        }

        // ---- Minimal demo response ----
        // For now: respond once we receive anything
        if (!read_buffer.empty() && write_buffer.empty())
        {
            write_buffer =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 5\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Hello";

            // enable EPOLLOUT
            update_epoll_events();
        }
    }

    // =========================
    // WRITE
    // =========================
    if (events & EPOLLOUT)
    {
        while (!write_buffer.empty())
        {
            ssize_t n = ::write(fd.get(), write_buffer.data(), write_buffer.size());

            if (n > 0)
            {
                write_buffer.erase(0, n);
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

                state = ConnectionState::CLOSE;
                return;
            }
        }

        // Finished writing
        if (write_buffer.empty())
        {
            state = ConnectionState::CLOSE;
        }

        update_epoll_events();
    }
}

bool Connection::should_close() const
{
    return state == ConnectionState::CLOSE;
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
