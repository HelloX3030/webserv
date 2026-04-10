#include "base/defines.hpp"
#include "base/logging.hpp"
#include "http/HttpMethods.hpp"

#include <cctype>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace WebServ
{

namespace
{

struct CgiEnv
{
    std::vector<std::string> storage;
    std::vector<char *> envp;
};

CgiEnv build_cgi_env(
    const std::filesystem::path &script_path,
    HttpMethod method,
    const std::string &target,
    const std::map<std::string, std::string> &headers,
    const std::string &body)
{
    CgiEnv result;

    auto &env = result.storage;

    env.push_back("REQUEST_METHOD=" + to_string(method));

    std::string query;
    size_t qpos = target.find('?');
    if (qpos != std::string::npos)
        query = target.substr(qpos + 1);

    env.push_back("QUERY_STRING=" + query);
    env.push_back("CONTENT_LENGTH=" + std::to_string(body.size()));

    std::map<std::string, std::string>::const_iterator it =
        headers.find("content-type");

    if (it != headers.end())
        env.push_back("CONTENT_TYPE=" + it->second);

    env.push_back("SCRIPT_FILENAME=" + script_path.string());
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");

    for (auto h = headers.begin(); h != headers.end(); ++h)
    {
        std::string key = h->first;

        for (size_t i = 0; i < key.size(); ++i)
        {
            if (key[i] == '-')
                key[i] = '_';
            else
                key[i] = std::toupper((unsigned char)key[i]);
        }

        env.push_back("HTTP_" + key + "=" + h->second);
    }

    for (size_t i = 0; i < env.size(); ++i)
        result.envp.push_back(const_cast<char *>(env[i].c_str()));

    result.envp.push_back(NULL);

    return result;
}

void apply_cgi_output_to_response(const std::string &output, HttpResponseBuilder &response)
{
    std::string::size_type header_end = output.find("\r\n\r\n");
    std::size_t delimiter_len = 4;

    if (header_end == std::string::npos)
    {
        header_end = output.find("\n\n");
        delimiter_len = 2;
    }

    // No CGI header block found: treat all output as body.
    if (header_end == std::string::npos)
    {
        response.set_body(output);
        return;
    }

    std::string header_block = output.substr(0, header_end);
    std::string body = output.substr(header_end + delimiter_len);

    std::size_t start = 0;
    while (start <= header_block.size())
    {
        std::size_t end = header_block.find('\n', start);
        std::string line;

        if (end == std::string::npos)
            line = header_block.substr(start);
        else
            line = header_block.substr(start, end - start);

        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (!line.empty())
        {
            std::size_t colon = line.find(':');
            if (colon != std::string::npos && colon > 0)
            {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);

                while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                    value.erase(0, 1);

                std::string lower_key = key;
                for (std::size_t i = 0; i < lower_key.size(); ++i)
                    lower_key[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower_key[i])));

                if (lower_key == "status")
                {
                    std::size_t pos = 0;
                    while (pos < value.size() && std::isdigit(static_cast<unsigned char>(value[pos])))
                        ++pos;

                    if (pos > 0)
                    {
                        int status_code = std::atoi(value.substr(0, pos).c_str());
                        response.set_status(status_code);
                    }
                }
                else
                {
                    response.set_header(key, value);
                }
            }
        }

        if (end == std::string::npos)
            break;

        start = end + 1;
    }

    response.set_body(body);
}

} // anonymous namespace

HttpResponseBuilder http_cgi(
    const std::filesystem::path &script_path,
    const std::string &interpreter,
    HttpMethod method,
    const std::string &target,
    const std::map<std::string, std::string> &headers,
    const std::string &body)
{

#ifdef DEBUG
    logging::log(HTTP_METHOD_CGI, "===== CGI INPUT BEGIN =====");

    logging::log(HTTP_METHOD_CGI, "script_path=\"" + script_path.string() + "\"");

    logging::log(HTTP_METHOD_CGI, "interpreter=\"" + interpreter + "\"");

    logging::log(HTTP_METHOD_CGI, "method=\"" + to_string(method) + "\"");

    logging::log(HTTP_METHOD_CGI, "target=\"" + target + "\"");

    logging::log(HTTP_METHOD_CGI, "headers_count=" + std::to_string(headers.size()));

    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end();
         ++it)
    {
        logging::log(HTTP_METHOD_CGI, "header: \"" + it->first + "\"=\"" + it->second + "\"");
    }

    logging::log(HTTP_METHOD_CGI, "body_size=" + std::to_string(body.size()));

    if (!body.empty())
    {
        logging::log(HTTP_METHOD_CGI, "body=\"" + body + "\"");
    }

    logging::log(HTTP_METHOD_CGI, "===== CGI INPUT END =====");
#endif

    // ---- check script exists ----
    if (!std::filesystem::exists(script_path))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_CGI, "Script does not exist -> 404");
#endif
        return HttpResponseBuilder(HttpStatus::NotFound);
    }

    CgiEnv env = build_cgi_env(script_path, method, target, headers, body);

    int in_pipe[2];
    int out_pipe[2];

    if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0)
        return HttpResponseBuilder(HttpStatus::InternalServerError);

    pid_t pid = fork();

    if (pid < 0)
        return HttpResponseBuilder(HttpStatus::InternalServerError);

    if (pid == 0)
    {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        close(in_pipe[1]);
        close(out_pipe[0]);

        // build argv
        char *argv[3];
        argv[0] = const_cast<char *>(interpreter.c_str());
        argv[1] = const_cast<char *>(script_path.c_str());
        argv[2] = NULL;

        execve(argv[0], argv, env.envp.data());

        // ---- exec failed ----
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    // write body
    if (!body.empty()) {
        ssize_t bytes_to_write = body.size();
        ssize_t bytes_written = write(in_pipe[1], body.data(), bytes_to_write);

        if (bytes_written == -1) {
            perror("write to CGI pipe failed");
        } else if (bytes_written < bytes_to_write) {
            std::cerr << "CGI: partial write to pipe (wrote " << bytes_written
                      << " of " << bytes_to_write << " bytes)\n";
        }
    }

    close(in_pipe[1]);

    // read output
    std::string output;
    char buffer[4096];

    ssize_t n;
    while ((n = read(out_pipe[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, n);

    close(out_pipe[0]);

    int status;
    waitpid(pid, &status, 0);

    // ---- child failed ----
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_CGI, "CGI execution failed -> 500");
#endif
        return HttpResponseBuilder(HttpStatus::InternalServerError);
    }

    // ---- empty output is invalid CGI ----
    if (output.empty())
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_CGI, "Empty CGI output -> 500");
#endif
        return HttpResponseBuilder(HttpStatus::InternalServerError);
    }

    HttpResponseBuilder result(HttpStatus::OK);
    apply_cgi_output_to_response(output, result);

    return result;
}

} // namespace WebServ
