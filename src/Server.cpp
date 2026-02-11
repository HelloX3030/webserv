#include "Server.hpp"

Server::Server()
    : server_fd(-1)
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
    }
    return *this;
}

Server::~Server()
{
}

void Server::start()
{
    log::log(SERVER, START);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        throw std::system_error(errno, std::system_category(), "Failed to create Socket");
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        int error = errno;
        perror("bind");
        close(server_fd);
        throw std::system_error(error, std::system_category(), "Failed to bind to adress");
    }

    if (listen(server_fd, 10) < 0)
    {
        int error = errno;
        perror("listen");
        close(server_fd);
        throw std::system_error(error, std::system_category(), "Failed to listen");
    }
}

void Server::stop()
{
    log::log(SERVER, STOP);
    close(server_fd);
}

void Server::run()
{
    start();

    while (g_running)
    {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
        {
            if (!g_running)
                break;
            perror("accept");
            continue;
        }

        char buffer[4096];
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n > 0)
        {
            // Print raw request to stdout
            write(STDOUT_FILENO, buffer, n);
        }

        const char response[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Hello";

        send(client_fd, response, sizeof(response) - 1, 0);
        close(client_fd);
    }

    stop();
}
