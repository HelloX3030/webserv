#include "core/signal.hpp"
#include "WebServ.hpp"
#include "base/defines.hpp"
#include "base/logging.hpp"
#include <sys/wait.h>

volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_sigint_count = 0;
volatile sig_atomic_t g_sigchld_pending = 0;

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
    // Mark that at least one child changed state.
    // Reaping is done in the main loop to avoid racing with code
    // that uses synchronous waitpid(pid, ...) (e.g. CGI).
    g_sigchld_pending = 1;
}
