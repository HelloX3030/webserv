## chunked transfer encoding: not implemented

### the question

should the parser handle `Transfer-Encoding: chunked`?

### the analysis

chunked encoding (RFC 9112 section 7.1) allows body transmission
without knowing total size upfront. each chunk is prefixed with
its hex length. a zero-length chunk terminates.

implementation requires:
- detecting `Transfer-Encoding: chunked` header
- parsing chunk headers (`size CRLF`)
- accumulating chunk data
- detecting trailer headers (optional)

complexity is moderate but non-trivial. many HTTP/1.1 clients use
Content-Length for simplicity. the 42 subject does not require chunked.

### the decision

not implemented. if `Transfer-Encoding: chunked` is present,
return 501 Not Implemented.

Content-Length is the only body-length mechanism.

### the principle

implement what the requirements demand.
chunked is useful but not required. defer to v2.







# update

from 42 subject.pdf

pg 11 of document
pg 12 of pdf file



Here are some specific remarks regarding CGIs:

for chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the body
