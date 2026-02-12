#include "classes/Server.hpp"

void Server::parse(const std::string &path)
{
    log::log(SERVER, PARSE_SERVER_CONFIG, path);
}
