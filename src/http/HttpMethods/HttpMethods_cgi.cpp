#include "base/defines.hpp"
#include "base/logging.hpp"
#include "http/HttpMethods.hpp"

#include <cctype>
#include <fcntl.h>
#include <filesystem>
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
        headers.find("Content-Type");

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
    if (!body.empty())
        write(in_pipe[1], body.data(), body.size());

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
    result.set_body(output);

    return result;
}

} // namespace WebServ
