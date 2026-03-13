// temp from Lukas, to replace with ghr's HttpResponseFrontend.hpp

#pragma once

#include "base/base.hpp"

class Connection;

class HttpParser
{
  private:
    std::string buffer;
    std::string response;

  public:
    HttpParser();
    HttpParser(const HttpParser &other);
    HttpParser &operator=(const HttpParser &other);
    ~HttpParser();

    // Functions
    void add_buffer(const Connection &connection, const char *buffer, ssize_t n);
    bool response_ready() const;
    std::string take_response();
    std::size_t get_buffer_size() const;
};
