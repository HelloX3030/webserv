#include "http/HttpMethods.hpp"

namespace WebServ
{

HttpResponse post(const ServerConfig &config, const std::string &path, const std::string &content)
{
    (void)config;
    (void)path;
    (void)content;
    return HttpResponse();
}

} // namespace WebServ
