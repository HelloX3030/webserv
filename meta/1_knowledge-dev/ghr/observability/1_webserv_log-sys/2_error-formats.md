# error formats

an error message is the sole output that survives to the operator.
by the time an exception reaches `main`, the call stack is gone —
line numbers, field values, and local context exist only if they
were embedded in the message at the throw site, where context is richest.

the message must therefore be self-sufficient at the point of
construction.


---


## the 4 required elements

a complete error message answers:

    what subsystem?    [config], [http] — scopes the message,
                       distinguishes config errors from runtime errors.

    where?             line number (parse-time), field value or
                       server index (validate-time), fd or address
                       (runtime). operator needs a location to act.

    what was expected? the grammar production, the valid range,
                       the required field.

    what was found?    the actual token value, the actual number,
                       the actual state.

not all 4 are always recoverable. validate-time errors lose token
line numbers — the struct carries field values, not source positions.
but at minimum: subsystem + location by whatever means available +
what was wrong.


---


## location availability by phase

location information is token-bound. once tokens become struct fields,
line numbers are gone.

    read        filepath available
    tokenise    no errors in this lexer (all byte sequences tokenise)
    parse       token line available
    interpret   token line available (value checked at parse time)
    validate    no line — struct only; identify by field value


---


## formats in ConfigFrontend

parse-time:

```
[config] line <N>: <message>
```

examples:
```
[config] line 12: expected ';'
[config] line 8:  unknown directive 'listen2'
[config] line 17: port out of range [1, 65535] — got '99999'
[config] line 23: invalid status code 'abc'
```

validate-time:

```
[config] validation error: <message>
```

examples:
```
[config] validation error: no server block defined
[config] validation error: server block has no listen directive
[config] validation error: location '/cgi-bin': cgi_extension and
    cgi_path must both be set or both absent
[config] validation error: error_page code out of range —
    got 600, valid range [100, 599]
```

the prefix `[config]` is constant across both formats. it scopes
every message to the config subsystem — distinguishable at a glance
from HTTP-layer or runtime errors.


---


## message discipline

operator-facing, not developer-facing.

name the offending construct: location path, directive name, value.
state what was expected and what was found, where both are known.
never expose internal type names: `TokenType`, `pos_`, `peek()`.
these are implementation details invisible to the operator and
meaningless for diagnosis.

the test: could the operator, reading only this message, identify
the exact line in their config file and understand what to fix?
if no: the message is insufficient.


---


## fail-fast vs accumulation

ConfigFrontend uses fail-fast: stop at first error, throw, exit.
the operator sees 1 precise error per run, fixes it, re-runs.

the alternative — accumulating errors across a parse run — requires
continuing after each error without corrupting subsequent state,
and filtering cascading errors (errors caused by prior errors).
for a config parser with fast operator feedback cycles, the added
complexity is not justified.

for a compiler processing large files, accumulation is valuable:
fix all errors in 1 edit/compile cycle. the trade-off resolves
differently at that scale.