#include "WebServ.hpp"

void quit()
{
    std::cout << "\nShutting down...\n";
    close(server_fd);
}
