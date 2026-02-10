#pragma once

#include "base.hpp"

namespace log
{

constexpr int log_title_width = 15;

void log(const char *title, const char *msg);
void log(const std::string &title, const char *msg);
void log(const char *title, const std::string &msg);
void log(const std::string &title, const std::string &msg);

} // namespace log
