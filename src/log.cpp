#include "log.hpp"

namespace log
{

void log(const std::string &title, const std::string &msg, LogType type)
{
#ifdef DEBUG
    if (title.length() > log_title_width - 2)
    {
        std::cout << "LOG TITLE TO LONG!" << std::endl;
    }
#endif

    int len = title.length();
    int left = (log_title_width - len) / 2;
    int right = log_title_width - len - left;

    if (type == LogType::NONE)
    {
        std::cout << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << msg << std::endl;
    }
    else
    {
        std::cerr << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << "ERROR: " << msg << std::endl;
    }
}

void log(const char *title, const char *msg, LogType type)
{
    log(std::string(title), std::string(msg), type);
}

void log(const std::string &title, const char *msg, LogType type)
{
    log(title, std::string(msg), type);
}

void log(const char *title, const std::string &msg, LogType type)
{
    log(std::string(title), msg, type);
}

} // namespace log
