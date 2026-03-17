#include "http/HttpRequestFrontend.hpp"

HttpRequestFrontend::HttpRequestFrontend(size_t max_body_size)
    : buffer_()
    , phase_(ParsePhase::REQUEST_LINE)
    , request_()
    , body_remaining_(0)
    , error_code_(0)
    , max_body_size_(max_body_size)
{
}

ParseResult HttpRequestFrontend::advance(const char*, size_t)
{
    return {ParseStatus::Incomplete, {}, 0};
}

void HttpRequestFrontend::reset()
{
    phase_ = ParsePhase::REQUEST_LINE;
    request_ = HttpRequest{};
    body_remaining_ = 0;
    error_code_ = 0;
}

bool HttpRequestFrontend::find_crlf(size_t&) const { return false; }
std::string_view HttpRequestFrontend::extract_line(size_t) const { return {}; }
void HttpRequestFrontend::consume_through(size_t) {}
