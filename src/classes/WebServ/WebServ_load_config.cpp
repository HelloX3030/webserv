#include "WebServ.hpp"
#include "classes/ConfigFrontend.hpp"

namespace WebServ
{

std::vector<ServerConfig> load_config(int argc, char **argv)
{
    if (argc > 2)
        throw std::runtime_error("usage: webserv [config_file]");
    const std::string path = (argc == 2) ? argv[1] : DEFAULT_CONFIG_PATH;
    try
    {
        return ConfigFrontend::parse(path);
    }
    catch (const std::exception &e)
    {
        logging::log(CONFIG_FRONTEND, e.what(), logging::LogType::ERROR);
        throw std::runtime_error(
            std::string(e.what()) + "\nusage: webserv [config_file]"); // re-throw for main to catch and exit
    }
}

} // namespace WebServ
