# implementation notes


## body handling

`request_.body` is `std::string` used as a byte buffer.

no parsing, no validation, no transformation of content.
chunked decoding removes transfer framing; the result is still raw octets.

interpretation belongs to handlers and CGI, not the frontend.


---


## header handling

### normalisation

field names normalised to lowercase during parsing.
downstream code operates on the canonical form.

### storage

headers stored in `std::map<std::string, std::string>`.
multiple headers with the same name are comma-joined per RFC 9110 §5.3.

### validation

`Content-Length` with differing values across multiple field-lines → 400.

`Host` required for HTTP/1.1; absence → 400.


---


## references

general HTTP semantics: `meta/2_knowledge/network-protocols/http/`
