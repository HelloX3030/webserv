#include "WebServ.hpp"

WebServ web_serv;

WebServ::WebServ()
{
}

WebServ::WebServ(const WebServ &other)
{
    *this = other;
}

WebServ &WebServ::operator=(const WebServ &other)
{
    if (this != &other)
    {
    }
    return *this;
}

WebServ::~WebServ()
{
}

void WebServ::parse(int argc, char **argv)
{
    // Parse Default Path
    if (argc == 1)
    {
        servers.emplace_back();
        servers[0].parse(DEFAULT_CONFIG_PATH);
    }

    // Parse Configs
    else
    {
        servers.resize(argc - 1);
        for (int i = 1; i < argc; i++)
        {
            servers[i].parse(argv[i]);
        }
    }
}

void WebServ::start()
{
}
