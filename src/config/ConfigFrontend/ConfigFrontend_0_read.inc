/* fragment: no includes, no guards. context: ConfigFrontend.cpp.

read filepath into string, replace # to end-of-line with spaces.

spaces not deletion: token line numbers are assigned at emission.
deletion shifts subsequent line numbers; spaces are invisible to the
lexer and preserve them.

precondition: unix line endings (\n). \r\n not handled — documented,
not checked. */
std::string Frontend::read(const std::string& filepath)
{
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec))
        throw std::runtime_error(
            "[config] cannot open file: '" + filepath + "'\n"
            "  reason: No such file or directory");

    if (std::filesystem::is_directory(filepath, ec))
        throw std::runtime_error(
            "[config] invalid config path: '" + filepath + "'\n"
            "  reason: path is a directory, expected a regular file");

    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error(
            "[config] cannot open file: '" + filepath + "'\n"
            "  verify path exists and is readable");

    file.exceptions(std::ifstream::badbit);

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
