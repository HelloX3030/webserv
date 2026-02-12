#include "WebServ.hpp"

int main(int argc, char **argv)
{
#ifdef DEBUG
    std::cout << "Test" << std::endl;
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

    WebServ::run();
    return 0;
}
