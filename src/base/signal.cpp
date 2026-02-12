#include "WebServ.hpp"

volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_sigint_count = 0;

void handle_sigint(int)
{
    ++g_sigint_count;

    if (g_sigint_count == 1)
    {
        log::log(SHUTDOWN, "Starting Graceful Shutdown...");
        g_running = 0;
    }
    else
    {
        log::log(SHUTDOWN, "Forcing shutdown...");
        std::_Exit(1);
    }
}
