/*
test_config.cpp — ConfigFrontend test driver.

parses a config file via ConfigFrontend::parse(),
prints the resulting ServerConfig structs via to_string().
validates the complete pipeline:
read → tokenise → parse → validate → serialise.

exit codes:
    0  success — config parsed and printed
    1  usage error or parse/validation failure
*/

#include "classes/ConfigFrontend.hpp"
#include "classes/Config.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    try
    {
        auto servers = ConfigFrontend::parse(argv[1]);

        std::cout << "parsed " << servers.size() << " server block"
                  << (servers.size() == 1 ? "" : "s") << "\n\n";

        for (size_t i = 0; i < servers.size(); ++i)
        {
            std::cout << "=== server block " << i << " ===\n";
            std::cout << to_string(servers[i]) << "\n\n";
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
