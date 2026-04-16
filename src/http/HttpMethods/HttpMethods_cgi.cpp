#include "base/defines.hpp"
#include "base/logging.hpp"
#include "http/HttpMethods.hpp"

#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <poll.h>
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

    std::string path_only = target;
    std::string query;
    size_t qpos = target.find('?');
    if (qpos != std::string::npos)
    {
        path_only = target.substr(0, qpos);
        query = target.substr(qpos + 1);
    }

    env.push_back("QUERY_STRING=" + query);
    env.push_back("CONTENT_LENGTH=" + std::to_string(body.size()));

    std::map<std::string, std::string>::const_iterator it =
        headers.find("content-type");

    if (it != headers.end())
        env.push_back("CONTENT_TYPE=" + it->second);

    std::string server_name = "localhost";
    std::string server_port = "8080";
    auto host_it = headers.find("host");
    if (host_it != headers.end() && !host_it->second.empty())
    {
        std::string host_value = host_it->second;
        size_t colon = host_value.rfind(':');
        if (colon != std::string::npos && colon + 1 < host_value.size())
        {
            server_name = host_value.substr(0, colon);
            server_port = host_value.substr(colon + 1);
        }
        else
        {
            server_name = host_value;
        }
    }

    env.push_back("SCRIPT_FILENAME=" + script_path.string());
    env.push_back("SCRIPT_NAME=" + path_only);
    env.push_back("PATH_INFO=" + path_only);
    env.push_back("REQUEST_URI=" + target);
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("SERVER_NAME=" + server_name);
    env.push_back("SERVER_PORT=" + server_port);

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

    // Stream request body to CGI stdin while simultaneously draining CGI stdout.
    // This avoids deadlocks for scripts that write output while still reading input.
    int in_flags = fcntl(in_pipe[1], F_GETFL, 0);
    int out_flags = fcntl(out_pipe[0], F_GETFL, 0);
    if (in_flags == -1 || out_flags == -1 ||
        fcntl(in_pipe[1], F_SETFL, in_flags | O_NONBLOCK) == -1 ||
        fcntl(out_pipe[0], F_SETFL, out_flags | O_NONBLOCK) == -1)
    {
        close(in_pipe[1]);
        close(out_pipe[0]);
        waitpid(pid, NULL, 0);
        return HttpResponseBuilder(HttpStatus::InternalServerError);
    }

    // Try to increase pipe buffer size for better throughput with large bodies
    int pipe_size = 1024 * 1024;  // 1MB
    fcntl(in_pipe[1], F_SETPIPE_SZ, pipe_size);
    fcntl(out_pipe[0], F_SETPIPE_SZ, pipe_size);

    size_t write_off = 0;
    bool stdin_open = true;
    bool stdout_open = true;

    if (body.empty())
    {
        close(in_pipe[1]);
        stdin_open = false;
    }

    std::string output;
    char buffer[4096];

    while (stdin_open || stdout_open)
    {
        struct pollfd fds[2];
        nfds_t nfds = 0;

        int in_idx = -1;
        int out_idx = -1;

        if (stdin_open)
        {
            in_idx = static_cast<int>(nfds);
            fds[nfds].fd = in_pipe[1];
            fds[nfds].events = (write_off < body.size()) ? POLLOUT : 0;
            fds[nfds].revents = 0;
            ++nfds;
        }

        if (stdout_open)
        {
            out_idx = static_cast<int>(nfds);
            fds[nfds].fd = out_pipe[0];
            fds[nfds].events = POLLIN | POLLHUP | POLLERR;
            fds[nfds].revents = 0;
            ++nfds;
        }

        if (poll(fds, nfds, -1) < 0)
        {
            if (errno == EINTR)
                continue;
            close(in_pipe[1]);
            close(out_pipe[0]);
            waitpid(pid, NULL, 0);
            return HttpResponseBuilder(HttpStatus::InternalServerError);
        }

        if (stdin_open && in_idx >= 0)
        {
            if (write_off >= body.size())
            {
                close(in_pipe[1]);
                stdin_open = false;
            }
            else if (fds[in_idx].revents & (POLLOUT | POLLERR | POLLHUP))
            {
                // Write in a loop while the pipe remains writable
                while (write_off < body.size())
                {
                    ssize_t n = write(in_pipe[1], body.data() + write_off, body.size() - write_off);
                    if (n > 0)
                    {
                        write_off += static_cast<size_t>(n);
                    }
                    else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    {
                        // Pipe buffer full, wait for next poll
                        break;
                    }
                    else if (n == -1 && errno == EINTR)
                    {
                        // Interrupted, retry
                        continue;
                    }
                    else
                    {
                        // Error
                        close(in_pipe[1]);
                        close(out_pipe[0]);
                        waitpid(pid, NULL, 0);
                        return HttpResponseBuilder(HttpStatus::InternalServerError);
                    }
                }

                if (write_off >= body.size())
                {
                    close(in_pipe[1]);
                    stdin_open = false;
                }
            }
        }

        if (stdout_open && out_idx >= 0)
        {
            if (fds[out_idx].revents & (POLLIN | POLLHUP | POLLERR))
            {
                while (true)
                {
                    ssize_t n = read(out_pipe[0], buffer, sizeof(buffer));
                    if (n > 0)
                        output.append(buffer, n);
                    else if (n == 0)
                    {
                        close(out_pipe[0]);
                        stdout_open = false;
                        break;
                    }
                    else if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    else if (errno == EINTR)
                    {
                        continue;
                    }
                    else
                    {
                        close(out_pipe[0]);
                        if (stdin_open)
                            close(in_pipe[1]);
                        waitpid(pid, NULL, 0);
                        return HttpResponseBuilder(HttpStatus::InternalServerError);
                    }
                }
            }
        }
    }

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
