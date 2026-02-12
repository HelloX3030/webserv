#include "classes/Server.hpp"

void Server::start()
{
    log::log(SERVER, START);

    // server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if (server_fd < 0)
    // {
    //     perror("socket");
    //     throw std::system_error(errno, std::system_category(), "Failed to create Socket");
    // }

    // sockaddr_in addr;
    // std::memset(&addr, 0, sizeof(addr));
    // addr.sin_family = AF_INET;
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // addr.sin_port = htons(8080);

    // if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    // {
    //     int error = errno;
    //     perror("bind");
    //     close(server_fd);
    //     throw std::system_error(error, std::system_category(), "Failed to bind to adress");
    // }

    // if (listen(server_fd, 10) < 0)
    // {
    //     int error = errno;
    //     perror("listen");
    //     close(server_fd);
    //     throw std::system_error(error, std::system_category(), "Failed to listen");
    // }
}
