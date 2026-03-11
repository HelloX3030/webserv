/* fragment: no includes, no guards. context: ConfigFrontend.cpp.

read filepath into string, replace # to end-of-line with spaces.

spaces not deletion: token line numbers are assigned at emission.
deletion shifts subsequent line numbers; spaces are invisible to the
lexer and preserve them.

precondition: unix line endings (\n). \r\n not handled — documented,
not checked. */
std::string Frontend::read(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error(
            "[config] cannot open file: '" + filepath + "'\n"
            "  verify path exists and is readable");
    std::string source(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>{});
    bool in_comment = false;
    for (char& c : source)
    {
        if (c == '#')                in_comment = true;
        if (c == '\n')               in_comment = false;
        if (in_comment && c != '\n') c = ' ';
    }

    return source;
}
