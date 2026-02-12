#include "Server.hpp"

void Server::stop()
{
    log::log(SERVER, STOP);
    // close(server_fd);
}
