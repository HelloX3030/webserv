#include "WebServ.hpp"

int main(int argc, char **argv)
{
    WebServ web_serv;

    // Signals Handling
    signal(SIGINT, handle_sigint);

    try
    {
        web_serv.parse(argc, argv);
    }
    catch (const std::exception &e)
    {
        log::log(PARSE_SERVER_CONFIG, e.what(), log::LogType::ERROR);
        return 1;
    }

    web_serv.start();
    return 0;
}
