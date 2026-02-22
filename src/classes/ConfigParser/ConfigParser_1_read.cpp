#include "../../../include/classes/ConfigParser.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

/* read file into memory & strip comments.

comment stripping replaces `#` to end-of-line 
with spaces, not deletion.
this preserves line numbers — every token produced
by lexer carries its line of origin. 
deleting chars would shift line numbers, breaking err msgs. 

precondition: source uses unix line endings (\n only).
\r\n (windows crlf) not handled */
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