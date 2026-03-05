#include "classes/Config.hpp"

bool ListenAddress::operator==(const ListenAddress& other) const
{
    return host == other.host && port == other.port;
}
