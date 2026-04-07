#include "http/HttpStatus.hpp"
#include <algorithm>
#include <cctype>

uint16_t to_code(HttpStatus status)
{
    return static_cast<uint16_t>(status);
}

std::string to_string(HttpStatus status)
{
    switch (status)
    {
    case HttpStatus::OK:
        return "OK";
    case HttpStatus::Created:
        return "Created";
    case HttpStatus::NoContent:
        return "No Content";
    case HttpStatus::MovedPermanently:
        return "Moved Permanently";
    case HttpStatus::Found:
        return "Found";
    case HttpStatus::SeeOther:
        return "See Other";
    case HttpStatus::BadRequest:
        return "Bad Request";
    case HttpStatus::Forbidden:
        return "Forbidden";
    case HttpStatus::NotFound:
        return "Not Found";
    case HttpStatus::MethodNotAllowed:
        return "Method Not Allowed";
    case HttpStatus::Conflict:
        return "Conflict";
    case HttpStatus::PayloadTooLarge:
        return "Payload Too Large";
    case HttpStatus::URITooLong:
        return "URI Too Long";
    case HttpStatus::RequestHeaderFieldsTooLarge:
        return "Request Header Fields Too Large";
    case HttpStatus::InternalServerError:
        return "Internal Server Error";
    case HttpStatus::NotImplemented:
        return "Not Implemented";
    case HttpStatus::HTTPVersionNotSupported:
        return "HTTP Version Not Supported";
    default:
        return "Unknown";
    }
}

std::optional<HttpStatus> http_status_from_code(uint16_t code)
{
    switch (code)
    {
    case 200:
        return HttpStatus::OK;
    case 201:
        return HttpStatus::Created;
    case 204:
        return HttpStatus::NoContent;
    case 301:
        return HttpStatus::MovedPermanently;
    case 302:
        return HttpStatus::Found;
    case 303:
        return HttpStatus::SeeOther;
    case 400:
        return HttpStatus::BadRequest;
    case 403:
        return HttpStatus::Forbidden;
    case 404:
        return HttpStatus::NotFound;
    case 405:
        return HttpStatus::MethodNotAllowed;
    case 409:
        return HttpStatus::Conflict;
    case 413:
        return HttpStatus::PayloadTooLarge;
    case 414:
        return HttpStatus::URITooLong;
    case 431:
        return HttpStatus::RequestHeaderFieldsTooLarge;
    case 500:
        return HttpStatus::InternalServerError;
    case 501:
        return HttpStatus::NotImplemented;
    case 505:
        return HttpStatus::HTTPVersionNotSupported;
    default:
        return std::nullopt;
    }
}

std::optional<HttpStatus> http_status_from_string(const std::string &value)
{
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    if (normalized == "200" || normalized == "ok")
        return HttpStatus::OK;
    if (normalized == "201" || normalized == "created")
        return HttpStatus::Created;
    if (normalized == "204" || normalized == "no content")
        return HttpStatus::NoContent;
    if (normalized == "301" || normalized == "moved permanently")
        return HttpStatus::MovedPermanently;
    if (normalized == "302" || normalized == "found")
        return HttpStatus::Found;
    if (normalized == "303" || normalized == "see other")
        return HttpStatus::SeeOther;
    if (normalized == "400" || normalized == "bad request")
        return HttpStatus::BadRequest;
    if (normalized == "403" || normalized == "forbidden")
        return HttpStatus::Forbidden;
    if (normalized == "404" || normalized == "not found")
        return HttpStatus::NotFound;
    if (normalized == "405" || normalized == "method not allowed")
        return HttpStatus::MethodNotAllowed;
    if (normalized == "409" || normalized == "conflict")
        return HttpStatus::Conflict;
    if (normalized == "413" || normalized == "payload too large")
        return HttpStatus::PayloadTooLarge;
    if (normalized == "414" || normalized == "uri too long")
        return HttpStatus::URITooLong;
    if (normalized == "431" || normalized == "request header fields too large")
        return HttpStatus::RequestHeaderFieldsTooLarge;
    if (normalized == "500" || normalized == "internal server error")
        return HttpStatus::InternalServerError;
    if (normalized == "501" || normalized == "not implemented")
        return HttpStatus::NotImplemented;
    if (normalized == "505" || normalized == "http version not supported")
        return HttpStatus::HTTPVersionNotSupported;

    return std::nullopt;
}
