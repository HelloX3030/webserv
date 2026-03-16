#include "net/Connection.hpp"
#include "base/defines.hpp"
#include "base/format.hpp"
#include "http/HttpMethods.hpp"
#include "net/Listener.hpp"
#include <sys/epoll.h>

Connection::~Connection()
{
}

Connection::Connection(Listener &listener, int fd)
    : fd(fd), state(ConnectionState::READ), write_offset(0), listener(listener), keep_alive(false)
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
            char buffer[WebServ::CONNECTION_READ_BUFFER_SIZE];

            ssize_t n = ::read(fd.get(), buffer, sizeof(buffer));

            if (n > 0)
            {
#ifdef DEBUG
                std::cout << format::header("Connection::handle_event::EPOLLIN buffer_start") << std::endl;
                std::cout << buffer;
                std::cout << format::header("Connection::handle_event::EPOLLIN buffer_end") << std::endl;
#endif
                http_parser.add_buffer(*this, buffer, n);
                if (http_parser.parse_error())
                {
                    state = ConnectionState::CLOSE;
                    break;
                }
            }
            else if (n == 0)
            {
                // peer closed connection
                state = ConnectionState::CLOSE;
                return;
            }
            else
            {
                if (errno == EINTR)
                    continue; // retry

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break; // no more data

                state = ConnectionState::CLOSE; // read error
                return;
            }
        }

        // load response if ready
        if (write_buffer.empty() && http_parser.response_ready())
        {
            write_buffer = http_parser.take_response();
            update_epoll_events();
        }
    }

    // =========================
    // WRITE
    // =========================
    if (events & EPOLLOUT)
    {
        while (write_offset < write_buffer.size())
        {
// write remaining buffer
#ifdef DEBUG
            std::cout << format::header("Connection::handle_event::EPOLLOUT buffer_start") << std::endl;
            std::cout << write_buffer.data() + write_offset;
            std::cout << format::header("Connection::handle_event::EPOLLOUT buffer_end") << std::endl;
#endif
            ssize_t n = ::write(fd.get(), write_buffer.data() + write_offset, write_buffer.size() - write_offset);

            if (n > 0)
            {
                write_offset += n; // advance offset
            }
            else
            {
                if (errno == EINTR)
                    continue; // retry

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break; // socket full

                state = ConnectionState::CLOSE; // write error
                return;
            }
        }

        // finished writing response
        if (write_offset == write_buffer.size())
        {
            write_buffer.clear();
            write_offset = 0;

            // next response if queued
            if (http_parser.response_ready())
            {
                write_buffer = http_parser.take_response();
            }
            else if (!keep_alive)
            {
                state = ConnectionState::CLOSE;
            }
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
    return "Connection(fd=" + std::to_string(get_fd()) + ", state=" + ::to_string(state) + ", read_buffer_size=" + std::to_string(http_parser.get_buffer_size()) + ", write_buffer_size=" + std::to_string(write_buffer.size()) + ", listener=" + listener.to_string() + ")";
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

const ServerConfig &Connection::get_default_server() const
{
    return listener.get_default_server();
}

const ServerConfig &Connection::get_server_config(const std::string &host) const
{
    const ServerConfig &config = listener.get_server_config(host);

#ifdef DEBUG
    logging::log(CONNECTION, "get_server_config");
    std::cout << ::to_string(config) << std::endl;
#endif

    return config;
}

void Connection::set_keep_alive()
{
    keep_alive = true;
}

namespace WebServ
{

void add_connection(Listener &listener, int fd)
{
    auto new_connection = std::make_unique<Connection>(listener, fd);
    add_epoll_handler(std::move(new_connection));
}

} // namespace WebServ
