#include "core/signal.hpp"
#include "WebServ.hpp"
#include "base/defines.hpp"
#include "base/logging.hpp"
#include <sys/wait.h>

volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_sigint_count = 0;

void handle_sigint(int)
{
    ++g_sigint_count;

    if (g_sigint_count == 1)
    {
        std::cout << std::endl;
        logging::log(SHUTDOWN, "Starting Graceful Shutdown...");
        g_running = 0;
    }
    else
    {
        std::cout << std::endl;
        logging::log(SHUTDOWN, "Forcing shutdown...");
        std::_Exit(1);
    }
}

void handle_sigchld(int)
{
    // Reap all zombie child processes
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}
