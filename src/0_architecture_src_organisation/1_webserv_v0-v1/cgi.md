## predicate

CGI process execution.
fork, exec, environment variables, pipe management, timeout handling.

isolated because:
- complex enough to warrant separation
- changes independently of HTTP and networking
- distinct failure modes (process lifecycle vs I/O)

---

## contents

not yet implemented.

potential files:
```
CgiHandler.cpp    fork/exec, pipe setup, environment construction
CgiProcess.cpp    child lifecycle, timeout, output capture
```

---

## naming

"cgi" — the technology/protocol being implemented.

---

## v0 → v1

new category. did not exist in v0.
