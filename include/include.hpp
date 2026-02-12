#pragma once

#include <csignal>
#include <cstring>
#include <iostream>
#include <vector>

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

#define DEBUG 1

// Defines
constexpr const char *START = "Start";
constexpr const char *STOP = "Stop";
constexpr const char *SHUTDOWN = "Shutdown";

// Log Titles
constexpr const char *WEB_SERV = "WebServ";
constexpr const char *SERVER = "Server";
constexpr const char *LISTENER = "Listener";

// Log Msg
constexpr const char *PARSE_SERVER_CONFIG = "Parse Server Config";

// defaults
constexpr const char *DEFAULT_CONFIG_PATH = "default/path";
