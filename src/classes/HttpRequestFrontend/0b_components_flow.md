

bytes on wire
    │
    ├─ framing       → CRLF-delimited lines
    ├─ lexer         → request-line tokens, header tokens
    ├─ parser        → HttpRequest struct
    └─ validator     → semantic checks



potential files:

1_cursor    // buffer pos. peek / consume (shame can't slurp)
2_request_line // Method SP URI SP Version CRLF
3_headers
4_body

    then have file sys as sgl src of truth, no need to represent here