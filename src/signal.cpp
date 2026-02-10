#include "signal.hpp"

volatile sig_atomic_t g_running = 1;

void handle_sigint(int)
{
    g_running = 0;
}
