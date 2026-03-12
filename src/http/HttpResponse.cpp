#include "http/HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse() : status(200)
{
}

HttpResponse::HttpResponse(int status) : status(status)
{
}

void HttpResponse::set_status(int s)
{
    status = s;
}

void HttpResponse::set_body(const std::string &b)
{
    body = b;
}

void HttpResponse::set_header(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

void HttpResponse::set_content_type(const std::string &type)
{
    headers["Content-Type"] = type;
}

std::string HttpResponse::to_string() const
{
    std::ostringstream response;

    response << WebServ::HTTP_VERSION << " " << status << " " << status_text(status) << "\r\n";

    // Headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        response << it->first << ": " << it->second << "\r\n";
    }

    if (headers.find("Content-Length") == headers.end())
        response << "Content-Length: " << body.size() << "\r\n";

    if (headers.find("Connection") == headers.end())
        response << "Connection: close\r\n";

    response << "\r\n";

    // Body
    response << body;

    return response.str();
}

std::string HttpResponse::status_text(int status)
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
