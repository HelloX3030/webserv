#include "WebServ.hpp"

int main(int argc, char **argv)
{
    // Signals Handling
    signal(SIGINT, handle_sigint);

    try
    {
        WebServ::parse(argc, argv);
    }
    catch (const std::exception &e)
    {
        log::log(WEB_SERV, e.what(), log::LogType::ERROR);
        return 1;
    }

    WebServ::start();
    return 0;
}
