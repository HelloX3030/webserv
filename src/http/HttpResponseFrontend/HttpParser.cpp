#include "http/HttpParser.hpp"
#include "http/HttpMethods.hpp"
#include "http/HttpResponseBuilder.hpp"
#include "net/Connection.hpp"
#include <utility>

void HttpParser::add_buffer(const Connection &connection, const char *buffer, ssize_t n)
{
    this->buffer.append(buffer, n);

    // Place Holder, for now just accumalate buffer + always respond

    // response = WebServ::post(connection.get_server_config("test"), "test", "test").to_string();
    (void)connection;
    response = HttpResponseBuilder().to_string();
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

bool HttpParser::parse_error() const
{
    return false;
}
