#include "net/Connection.hpp"
#include "base/defines.hpp"
#include "base/format.hpp"
#include "base/logging.hpp"
#include "http/HttpMethods.hpp"
#include "net/Listener.hpp"
#include <sys/epoll.h>

std::string to_string(ConnectionState state)
{
    switch (state)
    {
    case ConnectionState::ACTIVE:
        return ACTIVE;
    case ConnectionState::FAILED:
        return FAILED;
    case ConnectionState::CLOSE:
        return CLOSE;
    default:
        return UNKNOWN;
    }
}

Connection::Connection(Listener &listener, int fd)
    : fd(fd),
      state(ConnectionState::ACTIVE),
      http_request_frontend(listener.get_default_server().client_max_body_size),
      write_offset(0),
      listener(listener),
      keep_alive(false),
      peer_closed(false)
{
}

// Overrides
int Connection::get_fd() const
{
    return fd.get();
}

uint32_t Connection::get_events() const
{
    if (state == ConnectionState::FAILED)
        return EPOLLOUT;

    if (state == ConnectionState::CLOSE)
        return 0;

    uint32_t events = EPOLLIN;

    if (!write_buffer.empty() || !responses.empty())
        events |= EPOLLOUT;

    return events;
}

void Connection::handle_client_buffer(const char *buffer, ssize_t n)
{
    ParseResult result = http_request_frontend.advance(buffer, static_cast<size_t>(n));

    while (true)
    {
        if (result.status == ParseStatus::Incomplete)
            break;

        if (result.status == ParseStatus::Complete)
        {
            // IMPORTANT: reset per-request keep-alive
            keep_alive = false;

            HttpResponseBuilder response =
                WebServ::http_handle_request(*this,
                                             HttpMethod::GET,
                                             "abc",
                                             {},
                                             "Moin Moin");

            responses.push_back(response);

            http_request_frontend.reset();

            result = http_request_frontend.advance(nullptr, 0);
            continue;
        }

        // Failed
        keep_alive = false;

        HttpResponseBuilder response(result.error_code);
        responses.push_back(response);

        state = ConnectionState::FAILED;
        break;
    }
}

void Connection::handle_event(uint32_t events)
{
    // ---- Hard error ----
    if (events & EPOLLERR)
    {
        state = ConnectionState::CLOSE;
        return;
    }

    // peer performed shutdown(SHUT_WR)
    if (events & EPOLLRDHUP)
    {
        peer_closed = true;
    }

    // =========================
    // READ
    // =========================
    if ((events & EPOLLIN) && state != ConnectionState::FAILED)
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

                handle_client_buffer(buffer, n);

                if (state == ConnectionState::FAILED)
                    break;
            }
            else if (n == 0)
            {
                peer_closed = true;
                break;
            }
            else
            {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

                state = ConnectionState::CLOSE;
                return;
            }
        }

        // load response if ready
        if (write_buffer.empty() && !responses.empty())
        {
            for (std::size_t i = 0; i < responses.size(); i++)
                write_buffer += responses[i].to_string();

            responses.clear();
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
#ifdef DEBUG
            std::cout << format::header("Connection::handle_event::EPOLLOUT buffer_start") << std::endl;
            std::cout << write_buffer.data() + write_offset;
            std::cout << format::header("Connection::handle_event::EPOLLOUT buffer_end") << std::endl;
#endif

            ssize_t n = ::write(
                fd.get(),
                write_buffer.data() + write_offset,
                write_buffer.size() - write_offset);

            if (n > 0)
            {
                write_offset += n;
            }
            else
            {
                if (errno == EINTR)
                    continue;

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

                state = ConnectionState::CLOSE;
                return;
            }
        }

        // finished writing response
        if (write_offset == write_buffer.size())
        {
            write_buffer.clear();
            write_offset = 0;

            // FAILED must close after write
            if (state == ConnectionState::FAILED)
                state = ConnectionState::CLOSE;

            // load next queued responses (pipeline safe)
            else if (!responses.empty())
            {
                for (std::size_t i = 0; i < responses.size(); i++)
                    write_buffer += responses[i].to_string();

                responses.clear();
            }
            else
            {
                // decide connection lifetime
                if (!keep_alive)
                    state = ConnectionState::CLOSE;
                else if (peer_closed && write_buffer.empty())
                    state = ConnectionState::CLOSE;
            }
        }
    }

    update_epoll_events();
}

bool Connection::should_close() const
{
    return state == ConnectionState::CLOSE;
}

std::string Connection::to_string() const
{
    return "Connection(fd=" + std::to_string(get_fd()) +
           ", state=" + ::to_string(state) +
           ", listener=" + listener.to_string() + ")";
}

std::ostream &operator<<(std::ostream &os, const Connection &connection)
{
    return os << connection.to_string();
}

const ServerConfig &Connection::get_default_server_config() const
{
    return listener.get_default_server();
}

const ServerConfig &Connection::get_server_config(const std::string &host) const
{
    const ServerConfig &config = listener.get_server_config(host);
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
