// public: ParseResult, ParseStatus, struct decl + public methods

struct HttpRequestFrontend
{
    // state
    std::string buffer_;
    ParsePhase  phase_;
    HttpRequest request_;
    size_t      body_remaining_;
    uint16_t    error_code_;

    // public interface
    ParseResult advance(const char* data, size_t len);
    void reset();

private:
    // phase parsers — implementations in separate .cpp files
    PhaseResult parse_request_line();
    PhaseResult parse_header_line();
    PhaseResult consume_body();

    // helpers
    bool try_consume_crlf();
    std::string_view peek_line() const;
};
