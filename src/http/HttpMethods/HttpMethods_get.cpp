#include "base/defines.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "http/HttpMethods.hpp"
#include "http/HttpResponseBuilder.hpp"
#include <fstream>
#include <sstream>

namespace WebServ
{

static HttpResponseBuilder serve_file(const std::filesystem::path &file_path)
{
#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Serving file \"" + file_path.string() + "\"");
#endif

    std::ifstream file(file_path.c_str(), std::ios::binary);

    if (!file.is_open())
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "Failed to open file -> 500");
#endif
        return HttpResponseBuilder(HttpStatus::InternalServerError);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    HttpResponseBuilder res(HttpStatus::OK);
    res.set_body(buffer.str());

    res.set_content_type(file_path);

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, res.to_string());
#endif

    return res;
}

[[nodiscard]] HttpResponseBuilder
http_get(const std::filesystem::path &resolved_path,
         const std::vector<std::string> &index_files)
{
#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "resolved_path=\"" + resolved_path.string() + "\"");
#endif

    if (!std::filesystem::exists(resolved_path))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "File does not exist -> 404");
#endif
        return HttpResponseBuilder(HttpStatus::NotFound);
    }

    // directory handling
    if (std::filesystem::is_directory(resolved_path))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "Target is directory");
#endif

        // try index files
        for (std::vector<std::string>::const_iterator it = index_files.begin();
             it != index_files.end(); ++it)
        {
            std::filesystem::path index_path = resolved_path / *it;

            if (std::filesystem::exists(index_path) &&
                !std::filesystem::is_directory(index_path))
            {
#ifdef DEBUG
                logging::log(HTTP_METHOD_GET, "Serving index file \"" + index_path.string() + "\"");
#endif
                return serve_file(index_path);
            }
        }

#ifdef DEBUG
        logging::log(HTTP_METHOD_GET, "Directory without index -> 403");
#endif
        return HttpResponseBuilder(HttpStatus::Forbidden);
    }

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Serving file");
#endif

    return serve_file(resolved_path);
}

} // namespace WebServ
