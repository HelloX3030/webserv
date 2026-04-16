#include "net/Connection.hpp"
#include "base/defines.hpp"
#include "base/format.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "http/HttpMethods.hpp"
#include "net/Listener.hpp"

#include <chrono>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sys/ioctl.h>
#include <sstream>
#include <sys/epoll.h>

namespace
{

void apply_error_page_if_configured(const ServerConfig &config, HttpResponseBuilder &response)
{
    uint16_t status_code = response.get_status_code();

    if (status_code < 400 || status_code > 599)
        return;

    std::map<uint16_t, std::string>::const_iterator ep = config.error_pages.find(status_code);
    if (ep == config.error_pages.end())
        return;

    const std::string &error_uri = ep->second;

    utils::LocationMatch error_match = utils::match_location(config, error_uri);
    if (!error_match.location)
        return;

    std::string relative = error_uri.substr(error_match.prefix.size());
    if (!relative.empty() && relative[0] == '/')
        relative = relative.substr(1);

    std::optional<std::filesystem::path> safe = utils::resolve_path(error_match.location->root, relative);
    if (!safe)
        return;

    if (!std::filesystem::exists(*safe) || std::filesystem::is_directory(*safe))
        return;

    std::ifstream file(safe->c_str(), std::ios::binary);
    if (!file.is_open())
        return;

    std::stringstream buffer;
    buffer << file.rdbuf();

    response.set_body(buffer.str());
    response.set_content_type(*safe);
}

} // namespace

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
    peer_closed(false),
    last_activity(std::chrono::steady_clock::now())
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
        return (!write_buffer.empty() || !responses.empty()) ? EPOLLOUT : static_cast<uint32_t>(0);

    if (state == ConnectionState::CLOSE)
        return 0;

    uint32_t events = 0;

    if (!peer_closed)
        events |= EPOLLIN;

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
            bool request_keep_alive = result.request.keepAlive();

            // Under extreme large-upload load, forcing connection close after
            // response avoids stale keep-alive reuse and reduces EOF failures.
            long content_length = result.request.contentLength();
            if (content_length > 1024 * 1024)
                request_keep_alive = false;

            HttpResponseBuilder response = WebServ::http_handle_request(*this, result.request);
            response.set_header("Connection", request_keep_alive ? "keep-alive" : "close");

            responses.push_back(response);

            keep_alive = request_keep_alive;
            last_activity = std::chrono::steady_clock::now();

            // Once a request asks to close, stop consuming further pipelined input.
            // Remaining buffered bytes are ignored because the connection will close
            // after queued responses are written.
            if (!request_keep_alive)
                break;

            http_request_frontend.reset();

            result = http_request_frontend.advance(nullptr, 0);
            continue;
        }

        // Failed
        keep_alive = false;

        HttpResponseBuilder response(result.error_code);
        apply_error_page_if_configured(get_default_server_config(), response);
        response.set_header("Connection", "close");
        responses.push_back(response);

        state = ConnectionState::FAILED;
        break;
    }
}

void Connection::handle_event(uint32_t events)
{
    // Hard socket error: connection is not recoverable.
    if (events & EPOLLERR)
    {
        state = ConnectionState::CLOSE;
        return;
    }

    // Peer has closed (fully or write-half). Keep processing pending data/
    // responses instead of aborting immediately, otherwise clients can see EOF.
    if (events & (EPOLLRDHUP | EPOLLHUP))
    {
        peer_closed = true;
    }

    // =========================
    // READ
    // =========================
    if ((events & EPOLLIN) && state != ConnectionState::FAILED)
    {
        char buffer[WebServ::CONNECTION_READ_BUFFER_SIZE];
        while (true)
        {
            ssize_t n = ::read(fd.get(), buffer, sizeof(buffer));

            if (n > 0)
            {
#ifdef DEBUG
                std::cout << format::header("Connection::handle_event::EPOLLIN buffer_start") << std::endl;
                std::cout.write(buffer, n);
                std::cout << format::header("Connection::handle_event::EPOLLIN buffer_end") << std::endl;
#endif

                handle_client_buffer(buffer, n);
                last_activity = std::chrono::steady_clock::now();

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
            for (const HttpResponseBuilder &response : responses)
                write_buffer.append(response.to_string());

            responses.clear();
            update_epoll_events();
        }
    }

    // =========================
    // WRITE
    // =========================
    if ((events & EPOLLOUT) && !write_buffer.empty())
    {
        while (write_offset < write_buffer.size())
        {
#ifdef DEBUG
            std::cout << format::header("Connection::handle_event::EPOLLOUT buffer_start") << std::endl;
            std::cout.write(
                write_buffer.data() + write_offset,
                write_buffer.size() - write_offset);
            std::cout << format::header("Connection::handle_event::EPOLLOUT buffer_end") << std::endl;
#endif

            ssize_t n = ::write(
                fd.get(),
                write_buffer.data() + write_offset,
                write_buffer.size() - write_offset);

            if (n > 0)
            {
                write_offset += n;
                last_activity = std::chrono::steady_clock::now();
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
                for (const HttpResponseBuilder &response : responses)
                    write_buffer.append(response.to_string());

                responses.clear();
            }
            else
            {
                // decide connection lifetime
                if (!keep_alive || peer_closed)
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

bool Connection::has_timed_out(std::chrono::steady_clock::time_point now) const
{
    if (state == ConnectionState::CLOSE)
        return false;

    // If the kernel socket receive queue already has unread bytes,
    // this connection is making progress and must not be treated as idle.
    int pending = 0;
    if (::ioctl(fd.get(), FIONREAD, &pending) == 0 && pending > 0)
        return false;

    long long timeout_ms = static_cast<long long>(WebServ::CONNECTION_IDLE_TIMEOUT_MS);

    // Large uploads can legitimately pause while the single-threaded loop
    // handles other expensive requests (for example, CGI). Keep strict timeout
    // for small requests, but allow longer idle windows for large in-flight bodies.
    static const size_t LARGE_UPLOAD_THRESHOLD_BYTES = 1024 * 1024; // 1 MiB
    static const long long LARGE_UPLOAD_IDLE_TIMEOUT_MS = 180000;    // 180 s
    if (http_request_frontend.is_body_in_progress() &&
        http_request_frontend.expected_body_size() >= LARGE_UPLOAD_THRESHOLD_BYTES)
    {
        timeout_ms = LARGE_UPLOAD_IDLE_TIMEOUT_MS;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_activity);
    return elapsed.count() >= timeout_ms;
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
    return listener.get_server_config(host);
}

namespace WebServ
{

void add_connection(Listener &listener, int fd)
{
    auto new_connection = std::make_unique<Connection>(listener, fd);
    add_epoll_handler(std::move(new_connection));
}

} // namespace WebServ
