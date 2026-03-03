#include "base/format.hpp"

namespace format
{

std::string center(const std::string &str, int width, const char sep)
{
    int len = str.size();

    if (len >= width)
        return str;

    int left = (width - len) / 2;
    int right = width - len - left;

    return std::string(left, sep) + str + std::string(right, sep);
}

std::string header(const std::string &str)
{
    return center(str, 60, '=');
}

} // namespace format
