#include "http/HttpResponseBuilder.hpp"
#include "base/defines.hpp"
#include "base/errors.hpp"
#include <map>
#include <sstream>

namespace
{
HttpStatus checked_status_from_int(int code)
{
    if (code >= 0)
    {
        std::optional<HttpStatus> parsed = http_status_from_code(static_cast<uint16_t>(code));
        if (parsed.has_value())
            return *parsed;
    }

#ifdef DEBUG
    throw SetupError("Http Response code no status text found: " + std::to_string(code));
#else
    return HttpStatus::InternalServerError;
#endif
}
} // namespace

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

HttpResponseBuilder::HttpResponseBuilder(HttpStatus status) : status(status)
{
}

HttpResponseBuilder::HttpResponseBuilder(int status)
    : status(checked_status_from_int(status))
{
}

void HttpResponseBuilder::set_status(HttpStatus s)
{
    status = s;
}

void HttpResponseBuilder::set_status(int s)
{
    status = checked_status_from_int(s);
}

uint16_t HttpResponseBuilder::get_status_code() const
{
    return to_code(status);
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

    response << WebServ::HTTP_VERSION << " " << to_code(status) << " " << ::to_string(status) << "\r\n";

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
