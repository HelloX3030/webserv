# config frontend — components & flow


## concrete example

input:
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

output:

1 ServerConfig: host=0.0.0.0, port=8080, server_name=example.com,
1 location at "/" with root=/var/www/html and index=index.html.


---


## pipeline
```
file
 │
 │ read
 v
raw string
 │
 │ tokenise (strip comments: # to end of line)
 v
[Token, Token, Token, ...]
 │
 │ parse
 v
[ServerConfig, ...]   (fields populated, defaults applied)
 │
 │ validate
 v
[ServerConfig, ...]   (semantically verified)
 │
 v
WebServ::run()
```


---


## tokenisation example
```
server { listen 8080; }
```

becomes:
```
STRING("server")  LBRACE  STRING("listen")
STRING("8080")    SEMICOLON  RBRACE
```

`8080` tokenises as STRING, not NUMBER. grammar position determines
meaning — the parser interprets "8080" as a port when it follows "listen". 
the lexer classifies structure, not semantics.


---


## component boundaries
```
read:       does not interpret content
tokenise:   does not check meaning or validity
parse:      does not check semantic constraints
validate:   does not parse or interpret text
```

each component has 1 job. a bug is localised to exactly 1 component.