#include "WebServ.hpp"
#include "base/defines.hpp"
#include "base/logging.hpp"

int main(int argc, char **argv)
{
#ifdef DEBUG
    logging::log(INFORMATION, "DEBUG MODE ENABLED");
#endif

    // Signals Handling
    signal(SIGINT, handle_sigint);

    // Parse
    try
    {
        std::vector<ServerConfig> configs = WebServ::load_config(argc, argv);
        WebServ::init(configs);
#ifdef DEBUG
        WebServ::display();
#endif
        WebServ::run();
    }
    catch (const std::exception &e)
    {
        logging::log(WEB_SERV, e.what(), logging::LogType::ERROR);
        WebServ::quit();
        return 1;
    }
    WebServ::quit();
    return 0;
}
