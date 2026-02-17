#include "WebServ.hpp"

namespace WebServ
{

int init()
{
    if (Listener::init() != SUCCES)
    {
        Listener::quit();
        return FAILURE;
    }

    return SUCCES;
}

} // namespace WebServ
