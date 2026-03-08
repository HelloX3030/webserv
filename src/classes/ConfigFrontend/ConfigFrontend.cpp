#include "../../../include/classes/ConfigFrontend.hpp"

#include <string>
#include <fstream>
#include <iterator>
#include <stdexcept>

/* public entry point. the only public method.

pipeline execution:
    read      → std::string (raw content, comments stripped)
    tokenise  → populates tokens_
    parse     → std::vector<ServerConfig> (defaults applied)
    validate  → confirms semantic constraints

failure at any phase throws std::runtime_error with located message.
caller (main) catches once and exits. */
std::vector<ServerConfig> ConfigFrontend::parse(const std::string& filepath)
{
    std::string source = read(filepath);
    tokenise(source);
    std::vector<ServerConfig> result = parse_config();
    validate(result);
    return result;
}

/* phase 1: file read.

reads entire file into memory. strips comments.

comment stripping: replace # to end-of-line with spaces, not deletion.
whitespace replacement preserves line numbers — every token produced
by the lexer carries its true source line. deletion would shift line
numbers, breaking error messages.

precondition: source uses unix line endings (\n only).
\r\n (windows CRLF) not handled — would require normalisation pass.
the server runs on Linux; this precondition is documented, not checked. */
std::string ConfigFrontend::read(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error(
            "[config] cannot open file: '" + filepath + "'");

    std::string source(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>{});

    bool in_comment = false;
    for (char& c : source)
    {
        if (c == '#')
            in_comment = true;
        if (c == '\n')
            in_comment = false;
        if (in_comment && c != '\n')
            c = ' ';
    }

    return source;
}