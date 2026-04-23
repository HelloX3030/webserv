#pragma once

#include <signal.h>

extern volatile sig_atomic_t g_running;
extern volatile sig_atomic_t g_sigint_count;
extern volatile sig_atomic_t g_sigchld_pending;

void handle_sigint(int);
void handle_sigchld(int);
