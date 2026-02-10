#include "log.hpp"

namespace log
{

void log(const std::string &title, const std::string &msg)
{
    int len = title.length();
    int left = (log_title_width - len) / 2;
    int right = log_title_width - len - left;

    std::cout << "["
              << std::string(left, ' ')
              << title
              << std::string(right, ' ')
              << "] "
              << msg
              << std::endl;
}

void log(const char *title, const char *msg)
{
    log(std::string(title), std::string(msg));
}

void log(const std::string &title, const char *msg)
{
    log(title, std::string(msg));
}

void log(const char *title, const std::string &msg)
{
    log(std::string(title), msg);
}

} // namespace log
