#pragma once

#include "include.hpp"

extern volatile sig_atomic_t g_running;

void handle_sigint(int);
