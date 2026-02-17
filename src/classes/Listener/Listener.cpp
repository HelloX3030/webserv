#include "classes/Listener.hpp"
#include "classes/Server.hpp"

namespace Listener
{
int size = 0;
std::vector<int> server_id;
std::vector<int> port;
std::vector<int> fd;

void add(int new_server_id, int new_port)
{
    size++;
    server_id.push_back(new_server_id);
    port.push_back(new_port);
    fd.push_back(-1);
}

int init()
{
    for (int i = 0; i < size; i++)
    {
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0)
        {
            perror("socket");
            return FAILURE;
        }

        int opt = 1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("setsockopt");
            close(listen_fd);
            return FAILURE;
        }

        // struct sockaddr_in addr;
        // memset(&addr, 0, sizeof(addr));

        // addr.sin_family = AF_INET;          // IPv4
        // addr.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0 (all interfaces)
        // addr.sin_port = htons(8080);        // port 8080 (network byte order)

        // if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        //     perror("bind");
        //     close(listen_fd);
        //     exit(1);
        // }

        // /* =========================================================
        //  * 4. Mark the socket as a listening socket
        //  * ========================================================= */
        // if (listen(listen_fd, SOMAXCONN) < 0) {
        //     perror("listen");
        //     close(listen_fd);
        //     exit(1);
        // }

        fd[i] = listen_fd;
    }

    return SUCCES;
}

void quit()
{
    for (int i = 0; i < size; i++)
    {
        if (fd[i] != -1)
        {
            close(fd[i]);
        }
    }
}

} // namespace Listener
