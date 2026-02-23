#pragma once

#define WEBSERV_EPOLL_MAX_EVENTS 64
#define WEBSERV_EPOLL_TIMEOUT -1

// Exit Codes
constexpr const int SUCCES = 1;
constexpr const int FAILURE = -1;

// Structure
constexpr const char *BR = "-----------------------------------------------------------------------";
constexpr const char *ELLIPSIS = "...";

// Defines
constexpr const char *DEFAULT = "Default";
constexpr const char *ERROR = "Error";
constexpr const char *LIST = "List";
constexpr const char *INFORMATION = "Information";
constexpr const char *FUNCTION = "Function";
constexpr const char *START = "Start";
constexpr const char *STOP = "Stop";
constexpr const char *SHUTDOWN = "Shutdown";
constexpr const char *READ = "Read";
constexpr const char *WRITE = "Write";
constexpr const char *CLOSE = "Close";
constexpr const char *UNKNOWN = "Unknown";

// Log Titles
constexpr const char *DISPLAY = "Display";
constexpr const char *WEB_SERV = "WebServ";
constexpr const char *SERVER = "Server";
constexpr const char *LISTENER = "Listener";
constexpr const char *CONNECTION = "Connection";

// Log Msg
constexpr const char *PARSE_SERVER_CONFIG = "Parse Server Config";

// defaults
constexpr const char *DEFAULT_CONFIG_PATH = "default/path";
