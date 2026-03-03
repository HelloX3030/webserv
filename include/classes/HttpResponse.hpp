// temp from Lukas, to update
// I assume these are the fns he's started interfacing with? why? how?

#pragma once

#include "base/base.hpp"

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
    void add_buffer(const char *buffer, ssize_t n);
    bool response_ready() const;
    std::string take_response();
    std::size_t get_buffer_size() const;
};
