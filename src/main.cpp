#include "WebServ.hpp"

int main(int argc, char **argv)
{
    Server server;

    // Signals Handling
    signal(SIGINT, handle_sigint);

    try
    {
        server.parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        log::log("Parse Server Config", e.what(), log::LogType::ERROR);
    }

    server.run();
    return 0;
}
