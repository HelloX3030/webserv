/* tokenises source into tokens_.

finite automaton with 2 states: accumulating a STRING | not.
transitions on whitespace and structural characters.

token meaning is determined by grammar position, not token type —
"8080" is a port after "listen", a hostname after "server_name".
a NUMBER type would require the lexer to assign partial meaning, which
belongs to the parser. numeric values are consumed as STRING and
interpreted by parse_port / parse_size.

flush-before-emit: every boundary event terminates any STRING in
progress before emitting its own token. `flush` is a local procedure
closing over `current`, `tokens_`, and `line` — named to give the
operation identity at each call site; extracted to avoid 5 repetitions
of identical logic.

line tracking: incremented on \n only. each token receives its true
source line at emission.

END sentinel: appended unconditionally. postcondition: tokens_.back()
is always END. peek() is therefore safe at stream exhaustion —
tokens_[pos_] is always valid. */
void Frontend::tokenise(const std::string& source)
{
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
        if      (c == '\n')             { flush(); ++line; }
        else if (c == ' ' || c == '\t') { flush(); }
        else if (c == '{') { flush(); tokens_.push_back({TokenType::LBRACE,    "", line}); }
        else if (c == '}') { flush(); tokens_.push_back({TokenType::RBRACE,    "", line}); }
        else if (c == ';') { flush(); tokens_.push_back({TokenType::SEMICOLON, "", line}); }
        else               { current += c; }
    }
    flush();
    tokens_.push_back({TokenType::END, "", line});
}
