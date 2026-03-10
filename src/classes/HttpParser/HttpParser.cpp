#include "classes/HttpParser.hpp"
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

void HttpParser::add_buffer(const char *buffer, ssize_t n)
{
    this->buffer.append(buffer, n);

#ifdef DEBUG
    std::cout << format::header("HttpParser::buffer_start") << std::endl;
    std::cout << this->buffer << std::endl;
    std::cout << format::header("HttpParser::buffer_end") << std::endl;
#endif

    // Place Holder, for now just accumalate buffer + always respond
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
