#include "base/utils.hpp"
#include "base/defines.hpp"
#include "base/logging.hpp"
#include "config/Config.hpp"
#include <string>

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

LocationMatch match_location(const ServerConfig &config, const std::string &path)
{
    const Location *best_location = NULL;
    std::string best_prefix;

    for (std::map<std::string, Location>::const_iterator it = config.locations.begin(); it != config.locations.end(); ++it)
    {
        const std::string &prefix = it->first;

#ifdef DEBUG
        logging::log(HANDLE_REQUEST, "Checking location prefix=\"" + prefix + "\"");
#endif

        bool match = false;

        if (prefix == "/")
        {
            match = true; // root matches everything
        }
        else if (path.compare(0, prefix.size(), prefix) == 0)
        {
            if (prefix.back() == '/')
            {
                // prefix already enforces boundary
                match = true;
            }
            else
            {
                // enforce boundary (avoid /api matching /apix)
                if (path.size() == prefix.size() || path[prefix.size()] == '/')
                    match = true;
            }
        }

        if (match)
        {
#ifdef DEBUG
            logging::log(HANDLE_REQUEST, "Prefix match=\"" + prefix + "\"");
#endif

            if (prefix.size() > best_prefix.size())
            {
                best_location = &it->second;
                best_prefix = prefix;
            }
        }
    }

#ifdef DEBUG
    if (best_location)
    {
        logging::log(HANDLE_REQUEST, "Selected location prefix=\"" + best_prefix + "\" root=\"" + best_location->root + "\"");
    }
    else
    {
        logging::log(HANDLE_REQUEST, "No matching location found");
    }
#endif

    LocationMatch result;
    result.location = best_location;
    result.prefix = best_prefix;
    return result;
}

} // namespace utils
