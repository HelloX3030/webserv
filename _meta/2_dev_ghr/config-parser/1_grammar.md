# Configuration file: formal grammar

## formalism
ISO 14977 EBNF

## notation

Terminals are quoted strings. 
Whitespace between tokens is implicit and ignored.
Comments (`#` to end of line) are stripped before parsing.

---

## Grammar

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

### Server directives

```ebnf
listen_dir      = "listen", host_port, ";" ;
host_port       = port
                | host, ":", port ;

server_name_dir = "server_name", name, { name }, ";" ;
body_size_dir   = "client_max_body_size", size, ";" ;
error_page_dir  = "error_page", status_code, path, ";" ;
```

### Location directives

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

## Unspecified Directives

The subject specifies the following location-level behaviours
without prescribing directive names or syntax.
Names below are proposals — agree as team before implementing.

Redirection:

    return <status_code> <path> ;

    e.g. return 301 /new/path ;

File upload (two directives):

    upload_enable  <boolean> ;
    upload_store   <path> ;

These must be added to location_dir and the grammar
once names are agreed.

---

## Notes

`body_size_dir` appears in both server_dir and location_dir.
Server-level sets the default; location-level overrides it for that block.

`port` is unconstrained at the grammar level.
Valid range [1, 65535] is a semantic constraint, enforced in validation.

`status_code` is any three-digit sequence at the grammar level.
Validation checks against supported codes.

`path` must begin with `/`. This is a structural rule, not a semantic one.

Special character classes use ISO 14977 prose terminals (? ... ?),
the correct mechanism for character ranges that exceed pure BNF.