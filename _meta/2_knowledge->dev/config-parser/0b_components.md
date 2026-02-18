# Config Parser — Components

A bridge from the overview to the full grammar and plan.

---

## What the parser does, concretely

The operator writes a config file like this:

```
server {
    listen 8080;
    server_name example.com;

    location / {
        root /var/www/html;
        index index.html;
    }
}
```

The parser reads this file and produces one Server object:
host=0.0.0.0, port=8080, one location at "/" with root
/var/www/html and index file index.html.

The rest of the program never reads the config file.
It only ever sees the Server objects the parser produced.

---

## The problem the parser solves

The config file is text. The program needs structured data.
The gap between the two is what the parser bridges.

Text has no types, no structure the program can reason about.
A Server struct has typed fields, known ranges, clear ownership.
The parser performs this transformation once, at startup,
and guarantees the result is valid before the server runs.

---

## Four components

The parser is built from four components in sequence.
Each component is simple. Their composition does the work.

### 1. File reader

Reads the file into memory as a string.
Strips comments (# to end of line).

Input:  filepath
Output: raw string of config content

### 2. Lexer (tokeniser)

Breaks the raw string into a flat list of tokens.
A token is the smallest meaningful unit: a word, a number,
a brace, a semicolon.

The string:

    server { listen 8080; }

becomes:

    WORD("server")  LBRACE  WORD("listen")
    NUMBER("8080")  SEMICOLON  RBRACE

The lexer does not understand meaning.
It only recognises shape: is this a brace? a number? a word?
It records the line number of every token for error messages.

### 3. Parser

Reads the token list and builds Server objects.
It knows the grammar: what sequences of tokens are valid,
what each sequence means.

It sees WORD("server") followed by LBRACE and knows:
a server block is starting.
It sees WORD("listen") followed by NUMBER and knows:
this is the port.

When it encounters something invalid — unknown directive,
missing semicolon, wrong token where another was expected —
it throws immediately with the line number.

### 4. Validator

Runs after the full list of Server objects is built.
Checks constraints the grammar cannot express:

- does every server have at least one listen address?
- does every location have a root?
- is the port in [1, 65535]?
- if cgi_extension is set, is cgi_path also set?

Throws on any violation.

---

## Flow

```
file
 |
 | reader
 v
raw string
 |
 | lexer
 v
[Token, Token, Token, ...]
 |
 | parser
 v
[Server, Server, ...]   (with defaults applied)
 |
 | validator
 v
[Server, Server, ...]   (guaranteed valid)
 |
 v
WebServ::run()
```

---

## What each component does not do

Reader:   does not interpret content
Lexer:    does not check meaning or validity
Parser:   does not check semantic constraints
Validator: does not parse or interpret text

Each component has one job. This is why the system is
auditable: a bug is localised to exactly one component.