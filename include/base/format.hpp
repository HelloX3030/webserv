#pragma once

#include <string>

namespace format
{

std::string center(const std::string &str, int width, const char sep = ' ');
std::string header(const std::string &text);

} // namespace format
