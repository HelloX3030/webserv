## doc/

grammar
    ABNF — to keep direct correspondence with
    HTTP spec (RFC 7230/9110), which is written in ABNF


## potential files:

1_cursor    // buffer pos. peek / consume (shame can't slurp)
2_request_line // Method SP URI SP Version CRLF
3_headers
4_body