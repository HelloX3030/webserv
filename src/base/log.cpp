#include "base/log.hpp"
#include "base/defines.hpp"
#include "base/errors.hpp"

namespace logging
{

void log(std::string title, std::string msg, std::string value, LogType type)
{
#ifdef DEBUG
    if (title.length() > log_title_width - 2)
    {
        std::cout << "LOG TITLE TOO LONG!" << std::endl;
    }
#endif

    int len = title.length();
    int left = (log_title_width - len) / 2;
    int right = log_title_width - len - left;

    if (type == LogType::DEFAULT)
    {
        if (value == "")
            std::cout << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << msg << std::endl;
        else
            std::cout << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << msg << ": " << value << std::endl;
    }
    else if (type == LogType::ERROR)
    {
        if (value == "")
            std::cerr << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << "ERROR: " << msg << std::endl;
        else
            std::cerr << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << "ERROR: " << msg << ":" << value << std::endl;
    }
    else if (type == LogType::LIST)
    {
        if (value == "")
            std::cout << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << msg << std::endl;
        else
            std::cout << "[" << std::string(left, ' ') << title << std::string(right, ' ') << "] " << value << ": " << msg << std::endl;
    }
    else
    {
        throw SetupError("No Print Logic Implemented for " + to_string(type));
    }
}

void log(std::string title, std::string msg, LogType type)
{
    log(title, msg, "", type);
}

void log(std::string title, std::size_t i, std::string msg, LogType type)
{
    log(title, msg, std::to_string(i), type);
}

} // namespace log

std::string to_string(logging::LogType type)
{
    switch (type)
    {
    case logging::LogType::DEFAULT:
        return DEFAULT;
    case logging::LogType::ERROR:
        return ERROR;
    case logging::LogType::LIST:
        return LIST;
    default:
        return UNKNOWN;
    }
}
