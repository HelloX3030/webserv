#pragma once

#include <filesystem>
#include <optional>

struct ServerConfig;
struct Location;

namespace utils
{

// resolves all relative file system traversels and check if its still in base (e. g. base/../hacked would fail)
std::optional<std::filesystem::path> resolve_path(const std::string &base, const std::string &request_path);

struct LocationMatch
{
    const Location *location;
    std::string prefix;
};

// Select location for request path (defines base for URL → filesystem mapping)
LocationMatch match_location(const ServerConfig &config, const std::string &path);

} // namespace utils
