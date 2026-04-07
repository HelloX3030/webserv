#pragma once

#include <cstddef>

/*
whether something belongs in defines.hpp:
is this a value that:
(a) is referenced from multiple sites, or
(b) encodes a policy decision whose change
must propagate consistently across the system?
*/

/*
infrastructure policy constants:
system-wide tunables, potentially referenced from multiple sites,
encoding decisions about resource limits.
*/
// Batch Sizes
namespace WebServ
{

constexpr const char *HTTP_VERSION = "HTTP/1.0";
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
constexpr const char *WARNING = "Warning";
constexpr const char *ERROR = "Error";
constexpr const char *LIST = "List";
constexpr const char *INFORMATION = "Information";
constexpr const char *FUNCTION = "Function";
constexpr const char *START = "Start";
constexpr const char *STOP = "Stop";
constexpr const char *SHUTDOWN = "Shutdown";
constexpr const char *ACTIVE = "Active";
constexpr const char *FAILED = "Failed";
constexpr const char *CLOSE = "Close";
constexpr const char *UNKNOWN = "Unknown";

// Log Titles
constexpr const char *DISPLAY = "Display";
constexpr const char *WEB_SERV = "WebServ";
constexpr const char *CONFIG_FRONTEND = "ConfigFrontend";
constexpr const char *SERVER = "Server";
constexpr const char *LISTENER = "Listener";
constexpr const char *CONNECTION = "Connection";
constexpr const char *HANDLE_REQUEST = "HandleRequest";
constexpr const char *HTTP_PARSER = "HttpParser";
constexpr const char *EPOLL_HANDLER = "EPollHandler";
constexpr const char *HTTP_METHOD_GET = "HttpGET";
constexpr const char *HTTP_METHOD_POST = "HttpPOST";
constexpr const char *HTTP_METHOD_DELETE = "HttpDELETE";
constexpr const char *HTTP_METHOD_CGI = "HttpCGI";

// Log Msg
constexpr const char *PARSE_SERVER_CONFIG = "Parse Server Config";

// defaults
constexpr const char *DEFAULT_CONFIG_PATH = "config/valid/default.conf";

// Branch Prediction
#if defined(__GNUC__) || defined(__clang__)
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif
