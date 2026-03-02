# config frontend: configuration file — grammar


## notation

ISO 14977 EBNF.

why:
. formal specification of context-free grammar
. mathematical/formal language theory tradition
. simple

reading:
. terminals are quoted strings
. whitespace between symbols is implicit


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
                | body_size_dir
                | upload_enable_dir
                | upload_store_dir
                | return_dir ;
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
root_dir           = "root", path, ";" ;
index_dir          = "index", filename, { filename }, ";" ;
methods_dir        = "allowed_methods", method, { method }, ";" ;
autoindex_dir      = "autoindex", boolean, ";" ;
cgi_ext_dir        = "cgi_extension", extension, ";" ;
cgi_path_dir       = "cgi_path", path, ";" ;
upload_enable_dir  = "upload_enable", boolean, ";" ;
upload_store_dir   = "upload_store", path, ";" ;
return_dir         = "return", status_code, path, ";" ;
```


### terminals
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

prose terminals (? ... ?) per ISO 14977 for character ranges.


---


## notes

`body_size_dir` appears in both server_dir and location_dir.
server-level sets default; location-level overrides.

`port` unconstrained at grammar level.
valid range [1, 65535] enforced in validator.

`status_code` any 3-digit sequence at grammar level.
valid range [100, 599] enforced in validator.


### grammar vs validator boundary

the grammar expresses structure: `{ server_dir }` permits any subset
of directives, in any order, zero or more times.

the validator expresses necessity: some fields grammatically optional
are semantically mandatory.

`root` in location:
    grammar accepts location block with no root directive.
    validator rejects — a location without root cannot resolve paths.

`listen` in server:
    grammar accepts server block with no listen directive.
    validator rejects — a server with no address cannot bind.

general:
```
grammar:   can this sequence of tokens exist?
validator: does this completed struct represent something operational?
```

validator constraints (not grammar):
. mandatory field presence: root, listen
. cross-field coupling: cgi_extension ↔ cgi_path, return_code ↔ return_path
. value ranges: port [1, 65535], status_code [100, 599]