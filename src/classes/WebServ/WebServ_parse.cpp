#include "WebServ.hpp"

namespace WebServ
{

std::vector<ServerConfig> parse(int argc, char **argv)
{
    ConfigFrontend config_frontend;

    // Parse Default Path
    if (argc == 1)
    {
        return config_frontend.parse(DEFAULT_CONFIG_PATH);
    }

    // Parse Configs
    else if (argc == 2)
    {
        return config_frontend.parse(argv[1]);
    }

    // Args Error
    else
    {
        throw std::runtime_error("You need to provide exactly one config file path!");
    }
}

} // namespace WebServ
