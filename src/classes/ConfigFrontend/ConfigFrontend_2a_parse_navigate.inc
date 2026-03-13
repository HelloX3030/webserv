/* navigation primitives.

every parse_* function calls these. they are the sole interface
between the parser and the token vector.

peek/consume asymmetry:

    peek()    — const. pure observation. no state change.
    consume() — returns current token AND advances pos_.

peek() precedes a dispatch decision: at_STRING("location") checks
before committing. consume() follows a confirmed decision: the token
is already known correct.

both are safe at stream exhaustion: tokenise() postcondition
guarantees tokens_.back() is END. tokens_[pos_] is always a valid
access while the invariant holds.

invariant: no direct consume() call may be issued when pos_ ==
tokens_.size(). every call site is responsible for ensuring this —
either by a preceding peek() check, or by catching the END token in
expect() before any further access fires. */


/* return current token without advancing. */
Token Frontend::peek() const
{
    return tokens_[pos_];
}

/* true if current token is STRING with exactly this value.
used for keyword dispatch before committing a consume(). */
bool Frontend::at_STRING(const std::string& value) const
{
    Token t = peek();
    return t.type == TokenType::STRING && t.value == value;
}

/* return current token, advance pos_.
does not check type — caller is responsible for context. */
Token Frontend::consume()
{
    return tokens_[pos_++];
}

/* consume token, throw if type mismatches.

error message uses token appearance, not enum label:
"expected '{'" not "expected LBRACE" — operator-facing output. */
Token Frontend::expect(TokenType type)
{
    auto type_name = [](TokenType tp) -> std::string
    {
        switch (tp)
        {
            case TokenType::LBRACE:    return "'{'";
            case TokenType::RBRACE:    return "'}'";
            case TokenType::SEMICOLON: return "';'";
            case TokenType::STRING:    return "string";
            case TokenType::END:       return "end of input";
        }
        return "unknown";
    };

    Token t = consume();
    if (t.type == type)
        return t;
    throw std::runtime_error(
        "[config] line " + std::to_string(t.line) +
        ": expected " + type_name(type) + ", got '" +
        (t.type == TokenType::END ? "EOF" : t.value) + "'");
}

/* consume STRING or throw with grammar-position context.

not an alias for expect(TokenType::STRING).
error says "expected directive value" — what the grammar requires —
not "expected STRING" — an implementation detail. */
Token Frontend::expect_STRING()
{
    Token t = consume();
    if (t.type == TokenType::STRING)
        return t;
    throw std::runtime_error(
        "[config] line " + std::to_string(t.line) +
        ": expected directive value, got '" +
        (t.type == TokenType::END ? "EOF" : t.value) + "'");
}

/* consume SEMICOLON or throw. */
void Frontend::expect_SEMICOLON()
{
    expect(TokenType::SEMICOLON);
}