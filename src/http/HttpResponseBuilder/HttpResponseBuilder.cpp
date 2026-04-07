#include "http/HttpResponseBuilder.hpp"
#include "base/defines.hpp"
#include "base/errors.hpp"
#include <map>
#include <sstream>

std::string HttpResponseBuilder::status_text(int status)
{
    switch (status)
    {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 204:
        return "No Content";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 400:
        return "Bad Request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 409:
        return "Conflict";
    case 413:
        return "Payload Too Large";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    default:
#ifdef DEBUG
        throw SetupError("Http Response code no status text found: " + std::to_string(status));
#else
        return "Unknown";
#endif
    }
}

std::string HttpResponseBuilder::get_mime_type(const std::filesystem::path &path)
{
    static const std::map<std::string, std::string> types = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".gif", "image/gif"},
        {".txt", "text/plain"}};

    std::string ext = path.extension().string();

    std::map<std::string, std::string>::const_iterator it = types.find(ext);

    if (it != types.end())
        return it->second;

    return "application/octet-stream";
}

HttpResponseBuilder::HttpResponseBuilder(int status) : status(status)
{
}

void HttpResponseBuilder::set_status(int s)
{
    status = s;
}

void HttpResponseBuilder::set_body(const std::string &b)
{
    body = b;
}

void HttpResponseBuilder::set_header(const std::string &key, const std::string &value)
{
    headers.push_back(std::make_pair(key, value));
}

void HttpResponseBuilder::set_content_type(const std::filesystem::path &path)
{
    set_header("Content-Type", get_mime_type(path));
}

bool HttpResponseBuilder::has_header(const std::string &key) const
{
    for (std::vector<std::pair<std::string, std::string>>::const_iterator it = headers.begin();
         it != headers.end();
         ++it)
    {
        if (it->first == key)
            return true;
    }

    return false;
}

std::string HttpResponseBuilder::to_string() const
{
    std::ostringstream response;

    response << WebServ::HTTP_VERSION << " " << status << " " << status_text(status) << "\r\n";

    // Headers
    for (std::vector<std::pair<std::string, std::string>>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        response << it->first << ": " << it->second << "\r\n";
    }

    if (!has_header("Content-Length"))
        response << "Content-Length: " << body.size() << "\r\n";

    if (!has_header("Connection"))
        response << "Connection: close\r\n";

    response << "\r\n";

    // Body
    response << body;

    return response.str();
}
