#pragma once

#include <cstddef>

// Batch Sizes
namespace WebServ
{

constexpr const std::size_t EPOLL_MAX_EVENTS = 64;
constexpr const int EPOLL_TIMEOUT = -1;
constexpr const std::size_t EPOLL_HANDLERS_BATCH_SIZE = 64;
constexpr const std::size_t CONNECTION_READ_BUFFER_SIZE = 4096;

} // namespace WebServ

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
constexpr const char *HTTP_PARSER = "HttpParser";

// Log Msg
constexpr const char *PARSE_SERVER_CONFIG = "Parse Server Config";

// defaults
constexpr const char *DEFAULT_CONFIG_PATH = "default/path";

// Branch Prediction
#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif
