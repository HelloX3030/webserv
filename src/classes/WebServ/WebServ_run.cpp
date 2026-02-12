#include "WebServ.hpp"

namespace WebServ
{

void run()
{
    for (std::size_t i = 0; i < servers.size(); i++)
    {
        servers[i].run();
    }
}

} // namespace WebServ
