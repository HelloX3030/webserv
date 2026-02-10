#include <csignal>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

void handle_sigint(int)
{
    g_running = 0;
}

int main()
{
    signal(SIGINT, handle_sigint);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Listening on port 8080 (Ctrl-C to stop)\n";

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

    std::cout << "\nShutting down...\n";
    close(server_fd);
    return 0;
}
