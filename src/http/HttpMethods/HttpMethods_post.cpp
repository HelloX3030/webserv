
#include "base/defines.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "http/HttpMethods.hpp"
#include <fstream>

namespace WebServ
{

HttpResponseBuilder http_post(const std::filesystem::path &resolved_path, const std::string &content)
{
#ifdef DEBUG
    logging::log(HTTP_METHOD_POST, "resolved_path=\"" + resolved_path.string() + "\" content=\"" + content + "\"");
#endif

    auto parent = resolved_path.parent_path();

#ifdef DEBUG
    logging::log(HTTP_METHOD_POST, "Parent path=\"" + parent.string() + "\"");
#endif

    if (!std::filesystem::exists(parent))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_POST, "Parent directory does not exist -> 409");
#endif
        return HttpResponseBuilder(409);
    }

    if (std::filesystem::is_directory(resolved_path))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_POST, "Target path is a directory -> 403");
#endif
        return HttpResponseBuilder(403);
    }

    bool existed = std::filesystem::exists(resolved_path);

#ifdef DEBUG
    logging::log(HTTP_METHOD_POST, std::string("File existed=") + (existed ? "true" : "false"));
#endif

    std::ofstream file(resolved_path.c_str(), std::ios::binary);

    if (!file.is_open())
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_POST, "Failed to open file for writing -> 500");
#endif
        return HttpResponseBuilder(500);
    }

#ifdef DEBUG
    logging::log(HTTP_METHOD_POST, "Writing " + std::to_string(content.size()) + " bytes");
#endif

    file.write(content.data(), content.size());
    file.close();

#ifdef DEBUG
    logging::log(HTTP_METHOD_POST, existed ? "Returning 200 OK (overwrite)" : "Returning 201 Created");
#endif

    return HttpResponseBuilder(existed ? 200 : 201);
}

} // namespace WebServ
