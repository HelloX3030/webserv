#include "base/utils.hpp"

namespace utils
{

std::optional<std::filesystem::path> resolve_path(const std::string &base, const std::string &request_path)
{
    namespace fs = std::filesystem;

    fs::path base_path = fs::canonical(base);
    fs::path combined = base_path / request_path;

    fs::path normalized = combined.lexically_normal();

    if (normalized.string().compare(0, base_path.string().size(), base_path.string()) != 0)
        return std::nullopt; // escape attempt

    return normalized;
}

} // namespace utils
