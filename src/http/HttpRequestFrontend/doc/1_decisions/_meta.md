design choices and their justifications.

## summary of decisions

| aspect | decision | justification |
|--------|----------|---------------|
| lexer | hand-written | trivial regular structure |
| parser | state machine | no recursion, no nesting |
| representation | direct to struct | no intermediate tree needed |
| errors | fail-fast | no recovery possible in protocol |
| functions | member | intrinsic to object purpose |
| files | separate TUs | one concern per file |
| header case | normalise lowercase | canonical form simplifies access |
| CRLF | strict | enforce protocol correctness |
| chunked | not implemented | not required by spec |
| versions | 1.0 and 1.1 | text protocol scope |
