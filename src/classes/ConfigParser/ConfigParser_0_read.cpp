#include "../../../include/classes/ConfigParser.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

/* phase 1: read file into memory and strip comments.

comment stripping replaces # to end-of-line with spaces, not deletion.
whitespace replacement preserves line numbers — every token produced
by the lexer carries the line it was found on. deleting characters
would shift line numbers, breaking all error messages. 

precondition: source uses unix line endings (\n only).
\r\n (windows crlf) is not handled — \r would be accumulated
into STRING tokens, corrupting values like "8080\r" which
then fail numeric conversion. */
std::string ConfigParser::read(const std::string& filepath)
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