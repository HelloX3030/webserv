#include "../../../include/classes/ConfigParser.hpp"

#include <string>

std::vector<ServerConfig> ConfigParser::parse(const std::string& filepath)
{
    std::string source = read(filepath);
    tokenise(source);
    std::vector<ServerConfig> result = parse_config();
    validate(result);
    return result;
}