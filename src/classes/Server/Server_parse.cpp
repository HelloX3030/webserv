#include "classes/Server.hpp"

void Server::parse(const std::string &path)
{
    logging::log(SERVER, PARSE_SERVER_CONFIG, path);
}
