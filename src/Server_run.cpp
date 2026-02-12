#include "Server.hpp"

void Server::run()
{
    start();

    while (g_running)
    {
        // int client_fd = accept(server_fd, NULL, NULL);
        // if (client_fd < 0)
        // {
        //     if (!g_running)
        //         break;
        //     perror("accept");
        //     continue;
        // }

        // char buffer[4096];
        // ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        // if (n > 0)
        // {
        //     // Print raw request to stdout
        //     write(STDOUT_FILENO, buffer, n);
        // }

        // respond();
        // close(client_fd);
    }

    stop();
}
