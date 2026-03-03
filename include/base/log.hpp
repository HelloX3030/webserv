#pragma once

#include "include.hpp"

namespace log
{

constexpr int log_title_width = 20;

enum class LogType
{
    DEFAULT,
    ERROR,
    LIST,
};

void log(std::string title, std::string msg, std::string value, LogType type = LogType::DEFAULT);
void log(std::string title, std::string msg, LogType type = LogType::DEFAULT);
void log(std::string title, std::size_t i, std::string msg, LogType type = LogType::LIST);

} // namespace log

std::string to_string(log::LogType type);
