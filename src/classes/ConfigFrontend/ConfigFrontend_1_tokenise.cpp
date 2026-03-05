#include "../../../include/classes/ConfigFrontend.hpp"

#include <string>

/* tokenise source string into tokens_.

the lexer is a finite automaton with 2 states:
    accumulating a STRING | not.
whitespace and structural characters are the state transitions.

lexer rules:
    {            → LBRACE
    }            → RBRACE
    ;            → SEMICOLON
    anything else → STRING (accumulate until boundary)

structural chars are boundary events: they terminate any STRING in progress, 
then emit their own token. this is the flush-before-emit
invariant — no accumulated char is silently dropped.


why no NUMBER type:

"8080" after "listen" means port. 
"8080" after "server_name" would be a hostname. 
the token does not carry its own meaning — grammar
position determines interpretation. a NUMBER type would require the
lexer to classify digit-leading strings as semantically special,
violating sgl-responsibility: the lexer would be doing partial
meaning-assignment, which belongs to the parser.

consequence: directive parsers that expect numeric values consume
a STRING token and interpret via std::stoi. if conversion fails,
the parser throws with line number.


what the lexer does not do:
. validate values (port range, path existence)
. classify tokens by semantic meaning
. interpret size suffixes (10m, 1k)
. distinguish directive names from values or paths


line tracking:

line is incremented on \n only. every token receives the line number
at the moment of emission — the line the token appeared on, 
not relative to block or directive.


END sentinel postcondition:

tokenise() unconditionally appends Token{END, "", last_line}.
this makes peek() safe at stream exhaustion — no bounds check needed.
the parser loop terminates on END rather than testing pos_ against
tokens_.size(). postcondition: tokens_.back() is always END. */
void ConfigFrontend::tokenise(const std::string& source)
{
    tokens_.clear();
    pos_ = 0;   // reset for this parse
    // in this ctx, defensive, not necessary, as parse() not reused on same obj

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
            current += c;
        }
    }

    flush();
    tokens_.push_back({TokenType::END, "", line});
}