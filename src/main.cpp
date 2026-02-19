#include "WebServ.hpp"

int main(int argc, char **argv)
{
#ifdef DEBUG
    log::log(INFORMATION, "DEBUG MODE ENABLED");
#endif

    // Signals Handling
    signal(SIGINT, handle_sigint);

    // Parse
    try
    {
        WebServ::parse(argc, argv);
    }
    catch (const std::exception &e)
    {
        log::log(WEB_SERV, e.what(), log::LogType::ERROR);
        return 1;
    }

#ifdef DEBUG
    WebServ::add_test_data();
#endif

    // Init
    if (WebServ::init() != SUCCES)
    {
        WebServ::quit();
        return 1;
    }

#ifdef DEBUG
    WebServ::display();
#endif

    // Run
    try
    {
        WebServ::run();
    }
    catch (const std::exception &e)
    {
        log::log(WEB_SERV, e.what(), log::LogType::ERROR);
        WebServ::quit();
        return 1;
    }

    WebServ::quit();
    return 0;
}
