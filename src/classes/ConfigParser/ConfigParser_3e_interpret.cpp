#include "../../../include/classes/ConfigParser.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

/* STRING token → ListenAddress.
grammar: host_port = port | host, ":", port ;
colon presence distinguishes the 2 forms.
C++17 if-init scopes pos to the branch where it is meaningful. */
ListenAddress ConfigParser::parse_host_port(const Token& tok)
{
    ListenAddress addr;
    addr.host = "0.0.0.0"; // default: bind to all interfaces

    if (auto pos = tok.value.find(':'); pos != std::string::npos)
    {
        addr.host = tok.value.substr(0, pos);
        addr.port = parse_port(tok.value.substr(pos + 1), tok.line);
    }
    else
    {
        addr.port = parse_port(tok.value, tok.line);
    }
    return addr;
}

/* decimal string → uint16_t.
valid range [1, 65535]: 0 excluded because uint16_t admits it but
no valid service binds port 0.
stoi over stoul: stoi throws std::invalid_argument on non-numeric input;
stoul silently accepts leading whitespace and some edge inputs. */
uint16_t ConfigParser::parse_port(const std::string& s, size_t line)
{
    int n;
    try { n = std::stoi(s); }
    catch (...)
    {
        throw std::runtime_error(
            "[config] line " + std::to_string(line) +
            ": invalid port '" + s + "'");
    }
    if (n < 1 || n > 65535)
        throw std::runtime_error(
            "[config] line " + std::to_string(line) +
            ": port out of range [1, 65535]: " + s);
    return static_cast<uint16_t>(n);
}

/* STRING token value → size_t in bytes.
grammar: size = digit, { digit }, [ size_suffix ] ;
size_suffix = "k" | "K" | "m" | "M" | "g" | "G" ;

stoull over stoul: on 32-bit platforms size_t is 32 bits; stoull gives
64-bit precision before the cast, catching overflow stoul would truncate.
static_cast<unsigned char> on isdigit: char may be signed; passing a
negative value to isdigit is undefined behaviour. */
size_t ConfigParser::parse_size(const Token& tok)
{
    const std::string& s = tok.value;
    size_t i = 0;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
        ++i;

    if (i == 0)
        throw std::runtime_error(
            "[config] line " + std::to_string(tok.line) +
            ": invalid size value '" + s + "'");

    unsigned long long n;
    try { n = std::stoull(s.substr(0, i)); }
    catch (...)
    {
        throw std::runtime_error(
            "[config] line " + std::to_string(tok.line) +
            ": size value out of range '" + s + "'");
    }

    if (i == s.size())
        return static_cast<size_t>(n);

    if (i == s.size() - 1)
    {
        switch (s[i])
        {
            case 'k': case 'K': return static_cast<size_t>(n * 1024ULL);
            case 'm': case 'M': return static_cast<size_t>(n * 1024ULL * 1024ULL);
            case 'g': case 'G': return static_cast<size_t>(
                                    n * 1024ULL * 1024ULL * 1024ULL);
        }
    }

    throw std::runtime_error(
        "[config] line " + std::to_string(tok.line) +
        ": invalid size suffix in '" + s + "'");
}