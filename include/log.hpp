#pragma once

#include "includes.hpp"

namespace log
{

constexpr int log_title_width = 20;

enum class LogType
{
    NONE,
    ERROR,
};

void log(const char *title, const char *msg, LogType type = LogType::NONE);
void log(const std::string &title, const char *msg, LogType type = LogType::NONE);
void log(const char *title, const std::string &msg, LogType type = LogType::NONE);
void log(const std::string &title, const std::string &msg, LogType type = LogType::NONE);

} // namespace log
