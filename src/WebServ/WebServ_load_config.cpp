#include "WebServ.hpp"
#include "base/defines.hpp"
#include "base/logging.hpp"
#include "config/ConfigFrontend.hpp"

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
        throw std::runtime_error(e.what());
    }
}

} // namespace WebServ
