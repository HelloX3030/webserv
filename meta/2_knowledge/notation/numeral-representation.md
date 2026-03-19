# numeral representation in specifications

how protocol specifications denote numeric values.
the choice of radix (base) reflects alignment with data structures.


---


## ABNF radix prefixes

RFC 5234 defines 3 notations for literal byte values:
```abnf
%b01101000      ; binary
%d104           ; decimal
%x68            ; hexadecimal
```

all 3 denote the same value: ASCII 'h'.

the prefix is in-band meta-information:
the radix indicator is part of the value notation itself,
eliminating ambiguity without external context.


---


## why hexadecimal dominates

protocol specifications overwhelmingly use hex (%x).

**byte alignment** — 1 hex digit = 4 bits.
2 hex digits = 1 byte. clean mapping.
octal (3 bits per digit) misaligns with 8-bit bytes:
a byte requires 2⅔ octal digits.

**ASCII convention** — the ASCII table is traditionally
documented in hex (0x00–0x7F). protocol specs inherit this.
reading `%x0D` as "carriage return" becomes automatic.

**compactness** — `%x0D` is shorter than `%d13`,
more readable than `%b00001101`.


---


## octal: historical context

octal (base-8) appears in:
- Unix file permissions: `chmod 755`
- C escape sequences: `\015` (carriage return)
- PDP-11 era documentation

this traces to machines with word sizes divisible by 3:
12-bit, 18-bit, 36-bit words split cleanly into octal digits.
the PDP-11 had 16-bit words but inherited octal conventions
from its predecessors.

octal is absent from modern protocol specifications.
byte-oriented data favours hex.


---


## OCTET vs octal

a common confusion:

**OCTET** — from Latin *octo* (eight). means 8-bit byte.
the term exists because "byte" was historically ambiguous
(some architectures had 6-bit or 9-bit bytes).
IETF RFCs use "octet" for precision: exactly 8 bits.

**octal** — base-8 numeral system.

the words share etymology (both from "eight") but are unrelated:
OCTET counts bits, octal is a representation radix.
```abnf
message-body = *OCTET
```

reads: zero or more 8-bit bytes. no octal involved.


---


## ranges

ABNF supports ranges with hyphen:
```abnf
ALPHA = %x41-5A / %x61-7A   ; A-Z / a-z
DIGIT = %x30-39             ; 0-9
```

the range `%x41-5A` means all values from 0x41 to 0x5A inclusive.
this is why hex is preferred: reading `41-5A` as "A to Z"
requires knowing the ASCII table, but hex makes the
columnar structure visible (0x40 row = uppercase letters).


---


## other specifications

different specs use different conventions:

**Unicode** — `U+0041` for code points. the `U+` prefix
distinguishes Unicode scalars from raw bytes.

**C/C++** — `0x68` (hex), `0150` (octal with leading zero),
`0b01101000` (binary, C++14). the prefix varies by radix.

**JSON** — decimal only. no hex literals.
binary data must be encoded (base64).

**YAML** — `0x68` (hex), `0o150` (octal), decimal default.


---


## design principle

the choice of radix is not arbitrary:

- **hex** when byte-aligned data dominates (protocols, binary formats)
- **decimal** when human readability matters (Content-Length values)
- **binary** when bit-level structure is significant (flags, masks)
- **octal** only for historical compatibility (Unix permissions)

match the radix to the natural structure of the data.


---


## references

RFC 5234: Augmented BNF for Syntax Specifications
    https://www.rfc-editor.org/rfc/rfc5234
    section 2.3: terminal values

RFC 20: ASCII format for Network Interchange
    https://www.rfc-editor.org/rfc/rfc20

The Unicode Standard, Chapter 2: General Structure
    https://www.unicode.org/versions/latest/
