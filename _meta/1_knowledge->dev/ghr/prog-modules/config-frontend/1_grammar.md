# configuration file: formal grammar

## notation, formalism

ISO 14977 EBNF

why chosen?
appropriate for ctx:
. designed for formal specification of context-free grammar.
. from mathematical/formal language theory tradition.
    used in compiler textbooks, language specs. 
. simple (ideal for ghr as beginner informatics student)

(upcoming: for HTTP parsing: ABNF — to keep direct correspondence with
HTTP spec (RFC 7230/9110), which is written in ABNF)


Terminals are quoted strings. 
Whitespace between tokens is implicit and ignored.
Comments (`#` to end of line) are stripped before parsing.

---

## grammar

```ebnf
config          = server_block, { server_block } ;

server_block    = "server", "{", { server_dir }, "}" ;

server_dir      = listen_dir
                | server_name_dir
                | body_size_dir
                | error_page_dir
                | location_block ;

location_block  = "location", path, "{", { location_dir }, "}" ;

location_dir    = root_dir
                | index_dir
                | methods_dir
                | autoindex_dir
                | cgi_ext_dir
                | cgi_path_dir
                | body_size_dir ;
```

### server directives

```ebnf
listen_dir      = "listen", host_port, ";" ;
host_port       = port
                | host, ":", port ;

server_name_dir = "server_name", name, { name }, ";" ;
body_size_dir   = "client_max_body_size", size, ";" ;
error_page_dir  = "error_page", status_code, path, ";" ;
```

### location directives

```ebnf
root_dir        = "root", path, ";" ;
index_dir       = "index", filename, { filename }, ";" ;
methods_dir     = "allowed_methods", method, { method }, ";" ;
autoindex_dir   = "autoindex", boolean, ";" ;
cgi_ext_dir     = "cgi_extension", extension, ";" ;
cgi_path_dir    = "cgi_path", path, ";" ;
```

### Terminals

```ebnf
host            = ip_address | hostname ;
ip_address      = octet, ".", octet, ".", octet, ".", octet ;
octet           = digit, [ digit, [ digit ] ] ;
hostname        = label, { ".", label } ;
label           = letter, { letter | digit | "-" } ;
port            = digit, { digit } ;

size            = digit, { digit }, [ size_suffix ] ;
size_suffix     = "k" | "K" | "m" | "M" | "g" | "G" ;

status_code     = digit, digit, digit ;
method          = "GET" | "POST" | "DELETE" ;
boolean         = "on" | "off" ;
extension       = ".", letter, { letter | digit } ;

path            = "/", { path_char } ;
filename        = path_char, { path_char } ;
name            = printable_char, { printable_char } ;

path_char       = ? printable ASCII except whitespace, '{', '}', ';' ? ;
printable_char  = ? printable ASCII except whitespace ? ;
letter          = ? 'a'..'z' | 'A'..'Z' ? ;
digit           = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
```

---

## Notes

`body_size_dir` appears in both server_dir and location_dir.
Server-level sets the default; location-level overrides it for that block.

`port` is unconstrained at the grammar level.
Valid range [1, 65535] is a semantic constraint, enforced in validation.

`status_code` is any 3-digit sequence at the grammar level.
Validation checks against the full RFC 9110-defined range [100, 599].

`path` must begin with `/`. 
structural rule, semantic.

Special character classes use ISO 14977 prose terminals (? ... ?),
(mechanism for character ranges that exceed pure BNF).

---

### grammar permissiveness vs semantic necessity

A grammar rule expresses structural possibility, not operational
completeness. The grammar parser accepts any token sequence that
conforms to the production rules. It cannot reason about whether
the resulting struct is meaningful — that requires inspecting the
completed struct as a whole, which is the validator's function.

This creates a class of fields that are grammatically optional
but semantically mandatory. The grammar makes them optional because
the production rule `{ location_dir }` permits any subset of
directives, in any order, zero or more times — including zero.
The validator makes them required because the runtime cannot
function without them.

2 concrete cases in this program:

`root` in `location_dir`:
the grammar accepts a location block with no root directive.
the parser builds a `Location` with `root` as an empty string.
the validator rejects it: a location without root cannot resolve
any file path. the runtime would receive a struct that is
syntactically well-formed but operationally void.
the grammar could not enforce this without becoming context-sensitive
— it would need to inspect the *contents* of the block it just
parsed, not merely its structure. that is not a grammar's role.

`listen` in `server_dir`:
the grammar accepts a server block with no listen directive.
`{ server_dir }` permits zero iterations.
the validator rejects it: a server with no listen address has
nothing to bind to. the runtime cannot proceed.

The general principle:
```
grammar  : can this sequence of tokens exist?
validator: does this completed struct represent something real?
```

Constraints that belong to the validator, not the grammar:
. mandatory field presence (root, listen, locations)
. cross-field coupling (cgi_ext ↔ cgi_path, return_code ↔ return_path)
. value ranges that are semantic, not structural (port [1,65535], codes [100,599])

Constraints that belong to the grammar:
. structural ordering (server_dir before location_block is not enforced;
  both are elements of the same `{ server_dir }` loop)
. token-level syntax (path begins with `/`, boolean is `on` or `off`)

---

## implemented directives: decisions on unspecified syntax

The subject specification prescribes certain location-level
behaviours without prescribing directive names or syntax.
The following were unspecified; names and syntax were decided on 
by ghr and are now fixed in the implementation.

Redirection:

    return <status_code> <path> ;

    e.g. return 301 /new/path ;

File upload:

    upload_enable  <boolean> ;
    upload_store   <path> ;

These are included in `location_dir`.

Rationale for chosen names:
`return` mirrors nginx convention and reads as intent at the
call site. `upload_enable` / `upload_store` follow the boolean-flag
+ path-target pattern used by `autoindex` and `root` respectively,
maintaining internal grammatical consistency.

semantic coupling: `upload_enable on` requires `upload_store` to be
non-empty — an upload destination is operationally necessary.
`return_code` requires `return_path` non-empty — a redirect without
a target URI is malformed. both couplings are enforced in the
validator, not the grammar, for the reasons discussed above.