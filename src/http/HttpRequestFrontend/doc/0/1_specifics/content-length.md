How is Content-Length known?

HTTP does not automatically compute Content-Length. The sender must provide it.

When a client sends a POST request:

  POST /upload HTTP/1.1
  Host: example.com
  Content-Length: 13

  Hello, world!

The client knows the body size because it has the body in memory (or computed it beforehand).
It writes Content-Length: 13 into the headers, then writes the 13 bytes.


When we parse:

  We read headers until the empty line (CRLF CRLF)
  We find the Content-Length header, extract its value: "13"
  We parse "13" as an integer: 13
  We set body_remaining_ = 13
  We consume exactly 13 bytes from the stream


If the client lies (says 13, sends 10), we'll wait forever for the remaining 3 bytes (timeout, eventually).
If the client says 13 and sends 20, we consume 13 and leave 7 bytes in the buffer — either garbage or a pipelined next request.

This is why Content-Length accuracy is the client's responsibility. The protocol trusts it. The parser enforces it.

For chunked encoding, there's no Content-Length —
the body is sent in chunks, each prefixed with its size, terminated by a zero-length chunk.
This allows streaming without knowing total size upfront.
