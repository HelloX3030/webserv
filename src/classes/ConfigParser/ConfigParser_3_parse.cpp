#include "../../../include/classes/ConfigParser.hpp"

#include <string>

/* pipeline orchestrator.
pos_ reset before parse_config — method is re-entrant if needed,
though standard usage is construct-once, call-once, discard. */
std::vector<ServerConfig> ConfigParser::parse(const std::string& filepath)
{
    std::string source = read(filepath);
    tokenise(source);
    pos_ = 0;
    std::vector<ServerConfig> result = parse_config();
    validate(result);
    return result;
}