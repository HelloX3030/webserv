#include "../../../include/classes/ConfigParser.hpp"

#include <stdexcept>
#include <string>

/* parse: pipeline orchestrator.
pos_ reset before parse_config — method is re-entrant if needed,
though standard usage is construct-once, call-once, discard. */
std::vector<ServerConfig> ConfigParser::parse(const std::string& filepath)
{
    std::string source = read(filepath);
    tokenise(source);
    pos_ = 0;
    std::vector<ServerConfig> result = parse_config();
    validate(result);
    return result;
}

/* peek: return current token without advancing pos_.
const: pure observation, no state change.
safe at stream exhaustion: tokenise() postcondition guarantees
tokens_.back() is END. */
ConfigParser::Token ConfigParser::peek() const
{
    return tokens_[pos_];
}

/* at_STRING: true if current token is STRING with exactly this value.
used for named dispatch: at_STRING("location") in parse_server_block. */
bool ConfigParser::at_STRING(const std::string& value) const
{
    return peek().type == TokenType::STRING && peek().value == value;
}

/* consume: advance pos_, return the token that was current.
does not check type — caller is responsible for context. */
ConfigParser::Token ConfigParser::consume()
{
    return tokens_[pos_++];
}

/* expect: consume and return token, throw if type mismatches.
error names the token structurally for operator-facing messages,
not by enum constant name. */
ConfigParser::Token ConfigParser::expect(TokenType type)
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

/* expect_STRING: consume STRING or throw with grammar-position context.
error says "expected directive value" — what the grammar requires —
not "expected STRING", which is an implementation detail. */
ConfigParser::Token ConfigParser::expect_STRING()
{
    Token t = consume();
    if (t.type == TokenType::STRING)
        return t;
    throw std::runtime_error(
        "[config] line " + std::to_string(t.line) +
        ": expected directive value, got '" +
        (t.type == TokenType::END ? "EOF" : t.value) + "'");
}

/* expect_SEMICOLON: consume SEMICOLON or throw.
every directive ends with ';'. single enforcement point. */
void ConfigParser::expect_SEMICOLON()
{
    Token t = consume();
    if (t.type == TokenType::SEMICOLON)
        return;
    throw std::runtime_error(
        "[config] line " + std::to_string(t.line) +
        ": expected ';', got '" +
        (t.type == TokenType::END ? "EOF" : t.value) + "'");
}