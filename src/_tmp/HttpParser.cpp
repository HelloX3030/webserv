// currently in usage in Lukas' branch, needed for current build
// to be replaced by my implementation of HttpResponseFrontend

#include "classes/HttpParser.hpp"
#include "classes/Connection.hpp"
#include "http/HttpMethods.hpp"
#include "http/HttpResponse.hpp"

HttpParser::HttpParser()
{
}

HttpParser::HttpParser(const HttpParser &other)
{
    *this = other;
}

HttpParser &HttpParser::operator=(const HttpParser &other)
{
    if (this != &other)
    {
        buffer = other.buffer;
    }
    return *this;
}

HttpParser::~HttpParser()
{
}

void HttpParser::add_buffer(const Connection &connection, const char *buffer, ssize_t n)
{
    this->buffer.append(buffer, n);

    // Place Holder, for now just accumalate buffer + always respond

    // response = WebServ::post(connection.get_server_config("test"), "test", "test").to_string();
    (void)connection;
    response = HttpResponse().to_string();
}

bool HttpParser::response_ready() const
{
    return response != "";
}

std::string HttpParser::take_response()
{
    return std::exchange(response, "");
}

std::size_t HttpParser::get_buffer_size() const
{
    return buffer.size();
}
