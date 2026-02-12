#include "base/format.hpp"

namespace format
{

std::string center(const std::string &str, int width)
{
    int len = str.size();

    if (len >= width)
        return str;

    int left = (width - len) / 2;
    int right = width - len - left;

    return std::string(left, ' ') + str + std::string(right, ' ');
}

} // namespace format
