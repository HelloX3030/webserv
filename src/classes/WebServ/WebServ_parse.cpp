#include "WebServ.hpp"

namespace WebServ
{

std::vector<ServerConfig> parse(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    // TODO: uncomment, when frontend working
    // ConfigFrontend config_frontend;

    // // Parse Default Path
    // if (argc == 1)
    // {
    //     return config_frontend.parse(DEFAULT_CONFIG_PATH);
    // }

    // // Parse Configs
    // else if (argc == 2)
    // {
    //     return config_frontend.parse(argv[1]);
    // }

    // // Args Error
    // else
    // {
    //     throw std::runtime_error("You need to provide exactly one config file path");
    // }

    std::vector<ServerConfig> servers;

    /* ================================
       SERVER 1 (DEFAULT FOR 8080)
       ================================ */

    {
        ServerConfig config;

        config.listen = {
            ListenAddress{"127.0.0.1", 8080}};

        config.server_names = {
            "default_localhost"};

        config.client_max_body_size = 1048576;

        config.error_pages = {
            {404, "./errors/404.html"},
            {500, "./errors/500.html"}};

        Location root_location;
        root_location.root = "./www_server1";
        root_location.index_files = {"index.html"};
        root_location.allowed_methods = {HttpMethod::GET, HttpMethod::POST, HttpMethod::DELETE};
        root_location.autoindex = false;
        root_location.upload_enable = false;

        config.locations = {
            {"/", root_location}};

        servers.push_back(config);
    }

    /* ================================
       SERVER 2 (HOST MATCH TEST)
       ================================ */

    {
        ServerConfig config;

        config.listen = {
            ListenAddress{"127.0.0.1", 8080}};

        config.server_names = {
            "example.com"};

        config.client_max_body_size = 1048576;

        Location root_location;
        root_location.root = "./www/server2";
        root_location.index_files = {"index.html"};
        root_location.allowed_methods = {HttpMethod::GET};
        root_location.autoindex = true;
        root_location.upload_enable = false;

        config.locations = {
            {"/", root_location}};

        servers.push_back(config);
    }

    /* ================================
       SERVER 3 (NO SERVER_NAME)
       ================================ */

    {
        ServerConfig config;

        config.listen = {
            ListenAddress{"127.0.0.1", 8080}};

        config.server_names = {
            // intentionally empty
        };

        config.client_max_body_size = 1048576;

        Location root_location;
        root_location.root = "./www/server3";
        root_location.index_files = {"index.html"};
        root_location.allowed_methods = {HttpMethod::GET};
        root_location.autoindex = false;
        root_location.upload_enable = false;

        config.locations = {
            {"/", root_location}};

        servers.push_back(config);
    }

    /* ================================
       SERVER 4 (DIFFERENT PORT)
       ================================ */

    {
        ServerConfig config;

        config.listen = {
            ListenAddress{"127.0.0.1", 9090}};

        config.server_names = {
            "test.local"};

        config.client_max_body_size = 1048576;

        Location root_location;
        root_location.root = "./www/server4";
        root_location.index_files = {"index.html"};
        root_location.allowed_methods = {HttpMethod::GET};
        root_location.autoindex = false;
        root_location.upload_enable = false;

        config.locations = {
            {"/", root_location}};

        servers.push_back(config);
    }
    return servers;
}

} // namespace WebServ
