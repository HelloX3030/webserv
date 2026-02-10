#include "WebServ.hpp"

void WebServ::init()
{
    signal(SIGINT, handle_sigint);
}
