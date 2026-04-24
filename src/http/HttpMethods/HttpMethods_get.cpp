#include "base/defines.hpp"
#include "base/logging.hpp"
#include "base/utils.hpp"
#include "http/HttpMethods.hpp"
#include "http/HttpResponseBuilder.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <sstream>
#include <vector>

namespace WebServ
{

namespace
{

std::string html_escape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());

    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
    {
        switch (*it)
        {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += *it; break;
        }
    }

    return out;
}

bool is_unreserved_url_char(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '.' || c == '_' || c == '~';
}

std::string url_encode_path_segment(const std::string &s)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex;

    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
    {
        unsigned char c = static_cast<unsigned char>(*it);
        if (is_unreserved_url_char(c))
        {
            oss << static_cast<char>(c);
        }
        else
        {
            oss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }

    return oss.str();
}

HttpResponseBuilder serve_directory_listing(const std::filesystem::path &dir_path, const std::string &request_path)
{
    std::string base_url = request_path;
    if (base_url.empty())
        base_url = "/";
    if (base_url[base_url.size() - 1] != '/')
        base_url += "/";

    struct Entry
    {
        std::string name;
        bool is_dir;
    };

    std::vector<Entry> entries;
    try
    {
        for (std::filesystem::directory_iterator it(dir_path);
             it != std::filesystem::directory_iterator();
             ++it)
        {
            std::string name = it->path().filename().string();
            if (name == "." || name == "..")
                continue;

            bool is_dir = false;
            std::error_code ec;
            is_dir = it->is_directory(ec);
            if (ec)
                is_dir = false;

            entries.push_back(Entry{name, is_dir});
        }
    }
    catch (...)
    {
        return HttpResponseBuilder(HttpStatus::InternalServerError);
    }

    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        if (a.is_dir != b.is_dir)
            return a.is_dir > b.is_dir; // dirs first
        return a.name < b.name;
    });

    std::ostringstream html;
    html << "<!doctype html>\n";
    html << "<html><head><meta charset=\"utf-8\">";
    html << "<title>Index of " << html_escape(base_url) << "</title>";
    html << "</head><body>";
    html << "<h1>Index of " << html_escape(base_url) << "</h1>\n";
    html << "<ul>\n";

    if (base_url != "/")
        html << "<li><a href=\"../\">../</a></li>\n";

    for (std::vector<Entry>::const_iterator it = entries.begin(); it != entries.end(); ++it)
    {
        const std::string display = it->is_dir ? (it->name + "/") : it->name;
        const std::string href = url_encode_path_segment(it->name) + (it->is_dir ? "/" : "");
        html << "<li><a href=\"" << href << "\">" << html_escape(display) << "</a></li>\n";
    }

    html << "</ul>\n";
    html << "</body></html>\n";

    HttpResponseBuilder res(HttpStatus::OK);
    res.set_header("Content-Type", "text/html; charset=utf-8");
    res.set_body(html.str());
    return res;
}

} // namespace

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
         const std::vector<std::string> &index_files,
         bool autoindex,
         const std::string &request_path)
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

            if (!autoindex)
            {
        #ifdef DEBUG
                logging::log(HTTP_METHOD_GET, "Directory without index and autoindex off -> 403");
        #endif
                return HttpResponseBuilder(HttpStatus::Forbidden);
            }

        #ifdef DEBUG
            logging::log(HTTP_METHOD_GET, "Directory without index and autoindex on -> listing");
        #endif
            return serve_directory_listing(resolved_path, request_path);
    }

#ifdef DEBUG
    logging::log(HTTP_METHOD_GET, "Serving file");
#endif

    return serve_file(resolved_path);
}

} // namespace WebServ
