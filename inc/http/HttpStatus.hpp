#pragma once

#include <cstdint>
#include <optional>
#include <string>

enum class HttpStatus : uint16_t
{
    OK = 200,
    Created = 201,
    NoContent = 204,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    Conflict = 409,
    PayloadTooLarge = 413,
    URITooLong = 414,
    RequestHeaderFieldsTooLarge = 431,
    InternalServerError = 500,
    NotImplemented = 501,
    HTTPVersionNotSupported = 505
};

uint16_t to_code(HttpStatus status);
std::string to_string(HttpStatus status);

std::optional<HttpStatus> http_status_from_code(uint16_t code);
std::optional<HttpStatus> http_status_from_string(const std::string &value);
