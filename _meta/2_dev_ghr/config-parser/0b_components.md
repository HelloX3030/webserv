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

The parser reads this file and produces one ServerConfig object:
host=0.0.0.0, port=8080, one location at "/" with root
/var/www/html and index file index.html.

The rest of the program never reads the config file.
It only ever sees the ServerConfig objects the parser produced.

---

## The problem the parser solves

The config file is text. The program needs structured data.
The gap between the two is what the parser bridges.

Text has no types, no structure the program can reason about.
A ServerConfig struct has typed fields, known ranges, clear ownership.
The parser performs this transformation once, at startup,
and guarantees the result is valid before the server runs.

---

## Four components

The parser is built from 4 components in sequence.

### 1. File reader

Reads the file into memory as a string.
Strips comments (# to end of line).

Input:  filepath
Output: raw string of config content

### 2. Lexer (tokeniser)

Breaks the raw string into a flat list of tokens.
A token is the smallest meaningful unit: a STRING, a brace, a semicolon.

The string:

    server { listen 8080; }

becomes:

    STRING("server")  LBRACE  STRING("listen")
    STRING("8080")    SEMICOLON  RBRACE

The lexer does not understand meaning.
It recognises only structural boundaries: is this a brace? a semicolon?
anything else is a STRING. It records the line number of every token
for error messages.

Note: `8080` tokenises as STRING, not NUMBER. The lexer does not classify
digit-leading strings specially — that would require meaning-awareness,
which is the parser's job. The parser's directive helpers interpret the
STRING token's value string as a number when the grammar position requires it.

### 3. Parser

Reads the token list and builds ServerConfig objects.
It knows the grammar: what sequences of tokens are valid,
what each sequence means.

It sees STRING("server") followed by LBRACE and knows:
a server block is starting.
It sees STRING("listen") followed by STRING and knows:
this is a host:port or bare port — call parse_host_port().
parse_host_port() interprets the string and calls parse_port(),
which calls std::stoi. If the value is not numeric or out of range,
the parser throws with the line number.

When it encounters something invalid — unknown directive,
missing semicolon, wrong token where another was expected —
it throws immediately with the line number.

### 4. Validator

Runs after the full list of ServerConfig objects is built.
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
 | lexer (morphology)
 v
[Token, Token, Token, ...]
 |
 | parser (syntax)
 v
[ServerConfig, ServerConfig, ...]   (fields populated, defaults applied)
 |
 | validator (semantics)
 v
[ServerConfig, ServerConfig, ...]   (semantically verified)
 |
 v
WebServ::run()
```

---

## What each component does not do

Reader:    does not interpret content
Lexer:     does not check meaning or validity; does not classify by semantics
Parser:    does not check semantic constraints
Validator: does not parse or interpret text

Each component has one job. This is why the system is
auditable: a bug is localised to exactly one component.