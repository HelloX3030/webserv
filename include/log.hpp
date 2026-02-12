#pragma once

#include "include.hpp"

namespace log
{

constexpr int log_title_width = 20;

enum class LogType
{
    NONE,
    ERROR,
};

void log(std::string title, std::string msg, std::string value, LogType type = LogType::NONE);
void log(std::string title, std::string msg, LogType type = LogType::NONE);

} // namespace log
