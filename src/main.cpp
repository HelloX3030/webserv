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
        WebServ::init();
#ifdef DEBUG
        WebServ::add_test_data();
        WebServ::display();
#endif
        WebServ::run();
    }
    catch (const std::exception &e)
    {
        log::log(WEB_SERV, e.what(), log::LogType::ERROR);
        return 1;
    }
    return 0;
}
