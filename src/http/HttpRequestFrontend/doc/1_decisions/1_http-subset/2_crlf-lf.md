## bare LF tolerance: strict

### question

RFC 9112 requires CRLF as line terminator.
some implementations send bare LF.
should the parser tolerate it?

### analysis

tolerating bare LF:
- accepts non-conformant clients
- complicates delimiter detection (check for LF, optionally preceded by CR)
- masks client bugs

requiring CRLF:
- rejects non-conformant clients with 400
- simple delimiter detection (search for `\r\n`)
- enforces protocol correctness

the "be liberal in what you accept" principle (Postel's law) argues for tolerance.
experience shows this creates compatibility debt —
non-conformant implementations proliferate because they work.

### decision

strict. require CRLF. bare LF is 400 Bad Request.

### principle

strictness at boundaries prevents error propagation.
a parser that accepts garbage encourages garbage.
