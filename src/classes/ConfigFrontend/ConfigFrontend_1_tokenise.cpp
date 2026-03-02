#include "../../../include/classes/ConfigFrontend.hpp"

#include <string>

/* tokenise src string into tokens_.

the lexer is a finite automaton with 2 states: 
accumulating a STRING | not. 
whitespace & structural characters are the state transitions.

structural chars ({, }, ;) are boundary events: 
they terminate any STRING in progress, then emit their own token.

this ordering is the flush-before-emit invariant — 
no accumulated char is silently dropped.

line is incremented on \n only. structural chars and whitespace other
than \n do not change line. every token receives the line at the
moment of its emission (line token was on, not a line relative
to a block or directive) */
void ConfigFrontend::tokenise(const std::string& source)
{
    tokens_.clear();

    size_t      line = 1;
    std::string current;

    auto flush = [&]()
    {
        if (!current.empty())
        {
            tokens_.push_back({TokenType::STRING, current, line});
            current.clear();
        }
    };

    for (const char c : source)
    {
        if (c == '\n')
        {
            flush();
            ++line;
        }
        else if (c == ' ' || c == '\t')
        {
            flush();
        }
        else if (c == '{')
        {
            flush();
            tokens_.push_back({TokenType::LBRACE, "", line});
        }
        else if (c == '}')
        {
            flush();
            tokens_.push_back({TokenType::RBRACE, "", line});
        }
        else if (c == ';')
        {
            flush();
            tokens_.push_back({TokenType::SEMICOLON, "", line});
        }
        else
        {
            current += c;   // accumulate
        }
    }

    flush();
    tokens_.push_back({TokenType::END, "", line});
}