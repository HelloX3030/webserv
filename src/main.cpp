#include "WebServ.hpp"

int main(int argc, char **argv)
{
#ifdef DEBUG
    std::cout << format::header("DEBUG MODE ENABLED") << std::endl;
#endif

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

#ifdef DEBUG
    WebServ::add_test_data();
#endif

    if (WebServ::init() != SUCCES)
    {
        return 1;
    }

#ifdef DEBUG
    std::cout << "State after Parsing" << std::endl;
    WebServ::display();
#endif

    WebServ::run();
    WebServ::quit();

    return 0;
}
