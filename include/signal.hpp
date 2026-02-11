#pragma once

#include "include.hpp"

extern volatile sig_atomic_t g_running;
extern volatile sig_atomic_t g_sigint_count;

void handle_sigint(int);
