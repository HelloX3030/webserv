#pragma once

#include "http/HttpStatus.hpp"
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

class HttpResponseBuilder
{
  private:
    HttpStatus status = HttpStatus::OK;
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;

    bool has_header(const std::string &key) const;

    static std::string get_mime_type(const std::filesystem::path &path);

  public:
    HttpResponseBuilder() = default;
    HttpResponseBuilder(const HttpResponseBuilder &other) = default;
    HttpResponseBuilder &operator=(const HttpResponseBuilder &other) = default;
    ~HttpResponseBuilder() = default;

    // Special Constructor
    explicit HttpResponseBuilder(HttpStatus status);
    explicit HttpResponseBuilder(int status);

    // Functions
    void set_status(HttpStatus status);
    void set_status(int status);
    uint16_t get_status_code() const;
    void set_body(const std::string &body);
    void set_header(const std::string &key, const std::string &value);

    // Commom Header Types
    void set_content_type(const std::filesystem::path &path);

    std::string to_string() const;
};
