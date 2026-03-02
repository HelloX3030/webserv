#include "../../../include/classes/ConfigFrontend.hpp"

#include <stdexcept>
#include <string>

/* navigation primitives.

these functions navigate the token stream. every parse_* function
calls them. they are the interface between the parser and the
token vector.

the peek/consume asymmetry:

    peek()    — const. pure observation. returns current token
                without advancing. calling 10 times returns same token.
                
    consume() — not const. returns current token AND advances pos_.
                a side-effecting read.

peek() is used when a decision is needed before commitment:
    at_STRING("location") checks before consuming.
    
consume() is used when the token is known correct:
    after dispatch, the directive name is consumed and spent.

both are safe at stream exhaustion: tokenise() postcondition
guarantees tokens_.back() is END. no bounds check needed. */


/* return current token without advancing.
const: pure observation, no state change. */
ConfigFrontend::Token ConfigFrontend::peek() const
{
    return tokens_[pos_];
}

/* true if current token is STRING with exactly this value.
used for keyword dispatch: at_STRING("location"), at_STRING("server"). */
bool ConfigFrontend::at_STRING(const std::string& value) const
{
    return peek().type == TokenType::STRING && peek().value == value;
}

/* return current token, advance pos_.
does not check type — caller responsible for context.
not const: mutates pos_. */
ConfigFrontend::Token ConfigFrontend::consume()
{
    return tokens_[pos_++];
}

/* consume token, throw if type mismatches.

error message names the token structurally for operator-facing output,
not by enum constant. operator sees "expected '{'" not "expected LBRACE". */
ConfigFrontend::Token ConfigFrontend::expect(TokenType type)
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

expect_STRING() is not an alias for expect(TokenType::STRING).
it carries semantic context from the grammar position.

error says "expected directive value" — what the grammar requires —
not "expected STRING" — an implementation detail the operator
should not see. */
ConfigFrontend::Token ConfigFrontend::expect_STRING()
{
    Token t = consume();
    if (t.type == TokenType::STRING)
        return t;
    throw std::runtime_error(
        "[config] line " + std::to_string(t.line) +
        ": expected directive value, got '" +
        (t.type == TokenType::END ? "EOF" : t.value) + "'");
}

/* consume SEMICOLON or throw.

every directive ends with ';'. this is the single enforcement point.
error message is operator-facing: "expected ';'". */
void ConfigFrontend::expect_SEMICOLON()
{
    Token t = consume();
    if (t.type == TokenType::SEMICOLON)
        return;
    throw std::runtime_error(
        "[config] line " + std::to_string(t.line) +
        ": expected ';', got '" +
        (t.type == TokenType::END ? "EOF" : t.value) + "'");
}