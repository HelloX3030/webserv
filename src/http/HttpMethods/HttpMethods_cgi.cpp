#include "base/defines.hpp"
#include "base/logging.hpp"
#include "http/HttpMethods.hpp"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace WebServ
{

HttpResponseBuilder http_cgi(
    const std::filesystem::path &script_path,
    const std::string &interpreter,
    HttpMethod method,
    const std::string &target,
    const std::map<std::string, std::string> &headers,
    const std::string &body)
{
#ifdef DEBUG
    logging::log(HTTP_METHOD_CGI, "script=\"" + script_path.string() + "\" interpreter=\"" + interpreter + "\"");
#endif

    // ---- check script exists ----
    if (!std::filesystem::exists(script_path))
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_CGI, "Script does not exist -> 404");
#endif
        return HttpResponseBuilder(404);
    }

    int in_pipe[2];
    int out_pipe[2];

    if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0)
        return HttpResponseBuilder(500);

    pid_t pid = fork();

    if (pid < 0)
        return HttpResponseBuilder(500);

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

        // TODO build envp
        char *envp[] = {NULL};
        (void)method;
        (void)target;
        (void)headers;

        execve(argv[0], argv, envp);

        // ---- exec failed ----
        _exit(1);
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
        return HttpResponseBuilder(500);
    }

    // ---- empty output is invalid CGI ----
    if (output.empty())
    {
#ifdef DEBUG
        logging::log(HTTP_METHOD_CGI, "Empty CGI output -> 500");
#endif
        return HttpResponseBuilder(500);
    }

    HttpResponseBuilder result(200);
    result.set_body(output);

    return result;
}

} // namespace WebServ
