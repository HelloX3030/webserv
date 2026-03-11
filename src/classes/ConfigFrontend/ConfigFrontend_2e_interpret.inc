/* interpretation leaves.
these fns interpret STRING token values as typed values.
boundary between text (token.value) & semantics
(port number, byte size, host:port pair).

the parser consumes STRING tokens everywhere — the lexer has no NUMBER type.
these fn perform the string → typed value conversion,
throwing with line number on failure. */


/* interpret size with optional suffix.

suffixes: k/K (×1024), m/M (×1024²), g/G (×1024³).
no suffix: bytes.

used for `client_max_body_size` at both server & location level.

stoull over stoul: on 32-bit platforms size_t is 32 bits.
stoul would truncate before cast. stoull gives 64-bit precision. */
size_t ConfigFrontend::parse_size(const Token& t)
{
    const std::string& s = t.value;
    if (s.empty())
        throw std::runtime_error(
            "[config] line " + std::to_string(t.line) +
            ": empty size value");
    size_t multiplier = 1;
    std::string digits = s;
    char last = s.back();
    if (last == 'k' || last == 'K')
    {
        multiplier = 1024;
        digits = s.substr(0, s.size() - 1);
    }
    else if (last == 'm' || last == 'M')
    {
        multiplier = 1024 * 1024;
        digits = s.substr(0, s.size() - 1);
    }
    else if (last == 'g' || last == 'G')
    {
        multiplier = 1024 * 1024 * 1024;
        digits = s.substr(0, s.size() - 1);
    }
    size_t value;
    try { value = std::stoull(digits); }
    catch (...)
    {
        throw std::runtime_error(
            "[config] line " + std::to_string(t.line) +
            ": invalid size '" + s + "'");
    }
    if (value == 0)
        throw std::runtime_error(
            "[config] line " + std::to_string(t.line) +
            ": size must be > 0");
    // checked multiply: detect overflow before it wraps.
    if (multiplier > 1 && value > SIZE_MAX / multiplier)
        throw std::runtime_error(
            "[config] line " + std::to_string(t.line) +
            ": size '" + s + "' overflows size_t");
    return value * multiplier;
}

/* interpret port number.

valid range [1, 65535]. 0 is technically valid TCP but not useful
for a server to bind. range enforced here (parse-time rejection)
and confirmed in validator (belt-and-suspenders).

type: uint16_t. port space is [0, 65535]. 2^16 = 65536 > 65535.
uint16_t is the minimal standard width that fits. */
uint16_t ConfigFrontend::parse_port(const std::string& s, size_t line)
{
    int val;
    try { val = std::stoi(s); }
    catch (...)
    {
        throw std::runtime_error(
            "[config] line " + std::to_string(line) +
            ": expected port number, got '" + s + "'");
    }

    if (val < 1 || val > 65535)
        throw std::runtime_error(
            "[config] line " + std::to_string(line) +
            ": port out of range [1, 65535]: '" + s + "'");

    return static_cast<uint16_t>(val);
}

/* interpret host:port or bare port.

formats:
    "8080"           → host = "0.0.0.0", port = 8080
    "127.0.0.1:8080" → host = "127.0.0.1", port = 8080

host defaults to "0.0.0.0" (all interfaces) when absent.
colon presence distinguishes the two forms. */
ListenAddress ConfigFrontend::parse_host_port(const Token& t)
{
    ListenAddress addr;
    const std::string& s = t.value;

    size_t colon = s.find(':');
    if (colon == std::string::npos)
    {
        addr.host = DEFAULT_LISTEN_HOST;
        addr.port = parse_port(s, t.line);
    }
    else
    {
        addr.host = s.substr(0, colon);
        addr.port = parse_port(s.substr(colon + 1), t.line);
    }

    return addr;
}
