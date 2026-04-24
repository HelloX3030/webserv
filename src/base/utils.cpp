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

    // NOTE:
    // - canonical() throws if the path (or any component) doesn't exist.
    //   That breaks configs that point to directories created later.
    // - We still want symlink safety when the target exists.
    // Approach:
    //   1) Use absolute() (never requires existence).
    //   2) Use weakly_canonical() when possible (resolves symlinks for existing parts).
    //   3) Use lexically_relative() to enforce the resolved path stays within base.

    std::error_code ec;
    fs::path base_abs = fs::absolute(base, ec);
    if (ec)
        return std::nullopt;

    fs::path base_norm = fs::weakly_canonical(base_abs, ec);
    if (ec)
    {
        ec.clear();
        base_norm = base_abs.lexically_normal();
    }

    fs::path combined = (base_norm / request_path).lexically_normal();

    fs::path combined_norm = fs::weakly_canonical(combined, ec);
    if (!ec)
        combined = combined_norm;

    fs::path relative = combined.lexically_relative(base_norm);
    if (relative.empty() || relative.is_absolute())
        return std::nullopt; // unrelated path roots

    fs::path::iterator it = relative.begin();
    if (it != relative.end() && *it == "..")
        return std::nullopt; // escape attempt

    return combined;
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
