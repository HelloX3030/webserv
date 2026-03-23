#include "base/defines.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "http/HttpMethods.hpp"

namespace WebServ
{

HttpResponseBuilder http_delete(const std::filesystem::path &resolved_path)
{
#ifdef DEBUG
    logging::log(HTTP_METHOD_DELETE, "resolved_path=\"" + resolved_path.string() + "\"");
#endif

    if (!std::filesystem::exists(resolved_path))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_DELETE, "Target does not exist -> 404");
#endif
        return HttpResponseBuilder(404);
    }

    if (std::filesystem::is_directory(resolved_path))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_DELETE, "Target is directory -> 403");
#endif
        return HttpResponseBuilder(403);
    }

#ifdef DEBUG
    logging::log(HTTP_METHOD_DELETE, "Deleting file");
#endif

    try
    {
        if (!std::filesystem::remove(resolved_path))
        {
#ifdef DEBUG
            logging::log(HTTP_METHOD_DELETE, "remove() returned false -> 500");
#endif
            return HttpResponseBuilder(500);
        }
    }
    catch (...)
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_DELETE, "Filesystem deletion failed -> 500");
#endif
        return HttpResponseBuilder(500);
    }

#ifdef DEBUG
    logging::log(HTTP_METHOD_DELETE, "File deleted -> 200");
#endif

    return HttpResponseBuilder(200);
}

} // namespace WebServ
