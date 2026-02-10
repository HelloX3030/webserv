#include "WebServ.hpp"

int main(int argc, char **argv)
{
    ServerConfig server_config;
    try
    {
        server_config.parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        log::log("Parse ServerConfig", e.what(), log::LogType::ERROR);
    }

    // Run Server
    return 0;
}
