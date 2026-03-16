#pragma once

#include <filesystem>
#include <optional>

namespace utils
{

// resolves all relative file system traversels and check if its still in base (e. g. base/../hacked would fail)
std::optional<std::filesystem::path> resolve_path(const std::string &base, const std::string &request_path);

} // namespace utils
