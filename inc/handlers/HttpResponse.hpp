#pragma once

#include "base/base.hpp"

class HttpResponse
{
  private:
    int status;
    std::string body;
    std::map<std::string, std::string> headers;

    static std::string status_text(int status);
    static std::string get_mime_type(const std::filesystem::path &path);

  public:
    HttpResponse();
    HttpResponse(const HttpResponse &other) = default;
    HttpResponse &operator=(const HttpResponse &other) = default;
    ~HttpResponse() = default;

    // Special Constructor
    explicit HttpResponse(int status);

    // Functions
    void set_status(int status);
    void set_body(const std::string &body);
    void set_header(const std::string &key, const std::string &value);

    // Commom Header Types
    void set_content_type(const std::filesystem::path &path);

    std::string to_string() const;
};
