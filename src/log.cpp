#include "log.hpp"

namespace log
{

void log(std::string title, std::string msg, std::string value, LogType type)
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
        if (value == "")
            std::cout << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << msg << std::endl;
        else
            std::cout << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << msg << ": " << value << std::endl;
    }
    else
    {
        if (value == "")
            std::cerr << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << "ERROR: " << msg << std::endl;
        else
            std::cerr << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << "ERROR: " << msg << ":" << value << std::endl;
    }
}

void log(std::string title, std::string msg, LogType type)
{
    log(title, msg, "", type);
}

} // namespace log
