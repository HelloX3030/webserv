# failure semantics & response ("error handling")

program-wide
to extract from ConfigFrontend & integrate to _meta


## propagation contract

all components that can fail throw `std::runtime_error`.
`main` catches once and exits:
```cpp
catch (const std::exception& e) {
    log::log(WEB_SERV, e.what(), log::LogType::ERROR);
    return 1;
}
```

no error codes. no out-parameters. no per-layer catch-and-rethrow.
the message string is the sole carrier of error information.
what you put in the throw is what the operator reads.

---

## error sites and their formats

### parse-time
```
[config] line <N>: <message>
```

the lexer attaches a line number to every token.
the parser carries that line number to every throw site.
line numbers are the primary diagnostic tool: the operator can
open the file and go directly to the offending token.

examples:
```
[config] line 12: expected ';'
[config] line 8:  unknown directive 'listen2'
[config] line 17: port out of range [1, 65535]: '99999'
```

### validate-time
```
[config] validation error: <message>
```

the validator operates on completed structs — token line numbers
are no longer available. the struct carries field values, not
source positions. the message must identify the offending construct
by its content, not its location in the file.

examples:
```
[config] validation error: location '/' has no root directive
[config] validation error: server block has no listen directive
[config] validation error: location '/cgi-bin': cgi_extension and
    cgi_path must both be set or both absent
```

---

## message discipline

the message is the only output. operator-facing, not developer-facing.

requirements:
. name the offending construct: location path, directive name, value
. state what was expected and what was found, where possible
. never expose internal type names (TokenType, pos_, etc.)

the prefix `[config]` scopes the message to the config subsystem,
distinct from runtime errors produced elsewhere in the program.