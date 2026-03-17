# http request grammar

formal specification of HTTP/1.1 request syntax.
derived from RFC 9112 (HTTP/1.1 message syntax) and RFC 9110 (HTTP semantics).


---


## scope

this grammar covers HTTP/1.1 request messages only.
webserv parses requests; it generates responses.
response grammar is not specified here.

methods implemented: GET, POST, DELETE.
the grammar admits all methods; validation restricts to these 3.


---


## notation

ABNF (augmented Backus-Naur form) per RFC 5234.
```
rule       = definition          ; rule definition
"literal"                        ; case-sensitive string
%x0D                             ; hex byte value (CR)
*element                         ; 0 or more
1*element                        ; 1 or more
[optional]                       ; 0 or 1
(group)                          ; grouping
a / b                            ; alternatives
```


---


## core rules

from RFC 5234 appendix B.1:
```abnf
ALPHA   = %x41-5A / %x61-7A      ; A-Z / a-z
DIGIT   = %x30-39                ; 0-9
HTAB    = %x09                   ; horizontal tab
SP      = %x20                   ; space
VCHAR   = %x21-7E                ; visible (printing) characters
CR      = %x0D                   ; carriage return
LF      = %x0A                   ; line feed
CRLF    = CR LF                  ; internet standard newline
```


---


## http/1.1 request message

from RFC 9112 section 2.1:
```abnf
HTTP-message   = start-line CRLF
                 *( field-line CRLF )
                 CRLF
                 [ message-body ]
```

for requests, `start-line` is `request-line`.


---


## request line

from RFC 9112 section 3:
```abnf
request-line   = method SP request-target SP HTTP-version

method         = token
request-target = origin-form / absolute-form / authority-form / asterisk-form
HTTP-version   = "HTTP/" DIGIT "." DIGIT
```

webserv implements origin-form only:
```abnf
origin-form    = absolute-path [ "?" query ]
absolute-path  = 1*( "/" segment )
segment        = *pchar
query          = *( pchar / "/" / "?" )

pchar          = unreserved / pct-encoded / sub-delims / ":" / "@"
unreserved     = ALPHA / DIGIT / "-" / "." / "_" / "~"
pct-encoded    = "%" HEXDIG HEXDIG
sub-delims     = "!" / "$" / "&" / "'" / "(" / ")"
               / "*" / "+" / "," / ";" / "="
HEXDIG         = DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
                       / "a" / "b" / "c" / "d" / "e" / "f"
```


---


## header fields

from RFC 9110 section 5:
```abnf
field-line     = field-name ":" OWS field-value OWS
field-name     = token
field-value    = *field-content
field-content  = field-vchar [ 1*( SP / HTAB / field-vchar ) field-vchar ]
field-vchar    = VCHAR / obs-text
obs-text       = %x80-FF

OWS            = *( SP / HTAB )         ; optional whitespace
token          = 1*tchar
tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
               / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
               / DIGIT / ALPHA
```

note: `field-name` is case-insensitive per RFC 9110 section 5.1.
webserv normalises to lowercase during parsing.


---


## message body

from RFC 9112 section 6:
```abnf
message-body   = *OCTET
```

body length determined by:
1. `Content-Length` header: exactly that many bytes
2. `Transfer-Encoding: chunked`: chunked encoding (not implemented in webserv)
3. connection close: read until EOF (HTTP/1.0 fallback)

webserv implements Content-Length only.
absence of Content-Length with no body indication: body is empty.


---


## webserv subset

the full grammar is permissive. webserv restricts:
```
methods accepted:    GET, POST, DELETE
                     (others: 501 Not Implemented)

versions accepted:   HTTP/1.0, HTTP/1.1
                     (others: 505 HTTP Version Not Supported)

request-target:      origin-form only
                     (absolute-form, authority-form, asterisk-form: 400)

body encoding:       Content-Length only
                     (chunked: 501 Not Implemented)
```


---


## simplified production for implementation

collapsing the RFC grammar to what the parser actually recognises:
```abnf
request        = request-line *header-line CRLF [ body ]

request-line   = method SP uri SP version CRLF
method         = "GET" / "POST" / "DELETE" / token    ; token for error path
uri            = "/" *uri-char [ "?" *query-char ]
version        = "HTTP/1.0" / "HTTP/1.1"

header-line    = header-name ":" OWS header-value OWS CRLF
header-name    = 1*token-char
header-value   = *value-char

body           = <Content-Length octets>

; character classes
token-char     = ALPHA / DIGIT / "!" / "#" / "$" / "%" / "&" / "'"
               / "*" / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
uri-char       = unreserved / pct-encoded / sub-delims / ":" / "@" / "/"
query-char     = uri-char / "?"
value-char     = HTAB / SP / VCHAR / obs-text

OWS            = *( SP / HTAB )
CRLF           = %x0D %x0A
```


---


## language-theoretic classification

request-line: regular (type 3).
    fixed token sequence separated by SP, terminated by CRLF.
    no nesting, no recursion.

headers: regular (type 3).
    repetition of `header-line` is Kleene star.
    each line is a fixed pattern.
    no matching required across headers.

body: trivial.
    consume exactly n bytes where n = Content-Length.
    no structure to parse.

the only context-sensitive aspect: Content-Length value (in headers)
determines body length (later in stream). this is semantic, not syntactic.
the parser handles it by computing `body_remaining_` at HEADERS→BODY transition.

consequence: no stack needed. the "parser" is effectively a scanner
with phase state. recursive descent is unnecessary but harmless.


---


## delimiter semantics

CRLF as line terminator:
    RFC 9112 section 2.2 requires CRLF.
    bare LF: technically non-conformant but widely tolerated.
    webserv: strict — requires CRLF. bare LF is 400.

empty line as header terminator:
    CRLF immediately following the last header-line CRLF.
    this is the signal that headers are complete.
    implementation: detect CRLF at buffer start after consuming a header.

SP as token separator:
    exactly 1 SP between method/uri/version per RFC.
    multiple SP: non-conformant.
    webserv: strict — exactly 1 SP. multiple is 400.


---


## error conditions by grammar position

| position | violation | response |
|----------|-----------|----------|
| method | unrecognised token | 501 |
| method | empty or missing | 400 |
| SP after method | missing or multiple | 400 |
| uri | empty | 400 |
| uri | invalid characters | 400 |
| SP after uri | missing or multiple | 400 |
| version | not HTTP/1.x | 505 |
| version | malformed | 400 |
| CRLF after version | missing | 400 |
| header-name | empty | 400 |
| header colon | missing | 400 |
| Content-Length | non-numeric | 400 |
| Content-Length | negative | 400 |
| Content-Length | exceeds limit | 413 |
| body | fewer bytes than Content-Length | connection error |


---


## references

RFC 9110: HTTP Semantics
    https://www.rfc-editor.org/rfc/rfc9110

RFC 9112: HTTP/1.1
    https://www.rfc-editor.org/rfc/rfc9112

RFC 5234: ABNF
    https://www.rfc-editor.org/rfc/rfc5234

RFC 3986: URI Generic Syntax
    https://www.rfc-editor.org/rfc/rfc3986
