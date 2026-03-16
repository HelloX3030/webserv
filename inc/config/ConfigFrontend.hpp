#pragma once

#include "config/Config.hpp"

#include <string>
#include <vector>

/*
file: ConfigFrontend.hpp
module interface: 1 function over public types.
only consumer: main.

pipeline (internal — not visible here):

    std::string (filepath)
        │
        │ read        file → string, strip comments (# to eol)
        v
    std::string (raw content)
        │
        │ tokenise    string → token sequence
        v
    std::vector<Token>          (internal — never leaks here)
        │
        │ parse       tokens → structured config (recursive descent)
        v
    std::vector<ServerConfig>   (fields populated, defaults applied)
        │
        │ validate    semantic constraints the grammar cannot express
        v
    std::vector<ServerConfig>   (runtime-ready)

all pipeline types (TokenType, Token, Frontend) are internal to
ConfigFrontend.cpp, in an anonymous namespace.
no implementation detail appears in this header.
an includer that changes its Token representation triggers recompilation
of exactly 1 TU: ConfigFrontend.cpp.
*/
namespace ConfigFrontend
{
std::vector<ServerConfig> parse(const std::string &filepath);
}
