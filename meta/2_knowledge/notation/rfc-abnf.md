# IETF specification notation


## ABNF

Augmented Backus-Naur Form. defined in RFC 5234.
the metalanguage for IETF grammars.
```abnf
name       = definition          ; production rule
"literal"                        ; terminal string (case-sensitive by default)
%x0D                             ; terminal byte (hex)
%d13                             ; terminal byte (decimal)
%b1101                           ; terminal byte (binary)
%x30-39                          ; byte range (inclusive)
*rule                            ; repetition: 0 or more
1*rule                           ; repetition: 1 or more
n*mrule                          ; repetition: n to m occurrences
[rule]                           ; optional: 0 or 1
(a b)                            ; grouping: sequence
a / b                            ; alternation: a or b
```

ABNF is context-free. no recursion limit imposed by the notation,
though specific grammars may be regular (type 3).


---


## core rules

RFC 5234 appendix B.1 defines primitives used across specifications:
```abnf
ALPHA    = %x41-5A / %x61-7A     ; A-Z / a-z
DIGIT    = %x30-39               ; 0-9
HEXDIG   = DIGIT / "A"-"F" / "a"-"f"
OCTET    = %x00-FF               ; 8-bit byte
VCHAR    = %x21-7E               ; visible (printing) characters
SP       = %x20                  ; space
HTAB     = %x09                  ; horizontal tab
WSP      = SP / HTAB             ; whitespace
CR       = %x0D                  ; carriage return
LF       = %x0A                  ; line feed
CRLF     = CR LF                 ; internet standard newline
```

these are imported implicitly. no redefinition required.


---


## naming conventions

IETF names are terse. compression for repeated reference.

structural patterns:

| pattern          | example         | meaning                           |
|------------------|-----------------|-----------------------------------|
| component-form   | `path-absolute` | path in absolute form             |
| component-nz     | `segment-nz`    | segment, non-zero (non-empty)     |
| component-part   | `hier-part`     | hierarchical part                 |
| Xchar            | `pchar`         | character class for X (path char) |
| OXS              | `OWS`, `RWS`    | optional/required whitespace      |

lexical patterns:

| abbreviation | expansion                |
|--------------|--------------------------|
| pct          | percent                  |
| reg          | registered               |
| gen          | general                  |
| sub          | sub-component            |
| delims       | delimiters               |
| info         | information              |

the abbreviations are defined as ABNF productions.
first occurrence is the definition; subsequent are references.


---


## case sensitivity

ABNF rule names: case-insensitive by convention.
`URI`, `Uri`, `uri` refer to the same production.

string literals: case-sensitive by default in RFC 5234.
`"GET"` matches only `GET`, not `get`.

RFC 7405 introduces explicit case markers:
```abnf
%s"GET"     ; case-sensitive (default)
%i"get"     ; case-insensitive
```


---


## reading IETF specifications

1. locate the ABNF grammar — usually a dedicated section
2. trace from top-level production downward
3. expand abbreviations from their defining productions
4. cross-reference prose sections for semantic constraints
   ABNF captures syntax; prose captures meaning

the grammar is the primary source. prose elaborates.


---


## references

RFC 5234 — ABNF specification
RFC 7405 — case-sensitive string support in ABNF
RFC 3986, 9110, 9112 — exemplify the conventions
