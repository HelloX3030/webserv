#pragma once

#define WEBSERV_EPOLL_MAX_EVENTS 64
#define WEBSERV_EPOLL_TIMEOUT -1

// Exit Codes
constexpr const int SUCCES = 1;
constexpr const int FAILURE = -1;

// Defines
constexpr const char *INFORMATION = "Information";
constexpr const char *FUNCTION = "Function";
constexpr const char *START = "Start";
constexpr const char *STOP = "Stop";
constexpr const char *SHUTDOWN = "Shutdown";

// Log Titles
constexpr const char *DISPLAY = "Display";
constexpr const char *WEB_SERV = "WebServ";
constexpr const char *SERVER = "Server";
constexpr const char *LISTENER = "Listener";

// Log Msg
constexpr const char *PARSE_SERVER_CONFIG = "Parse Server Config";

// defaults
constexpr const char *DEFAULT_CONFIG_PATH = "default/path";
