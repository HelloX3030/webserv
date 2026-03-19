# http status codes

3-digit integers.

range: total defined space per HTTP spec (RFC 9110, 2022): [100, 599]

leading digit — class:
```
1xx — informational
2xx — success
3xx — redirection
4xx — client error
5xx — server error
```

remaining 2 digits — specific subclass code within that class.

codes <100 or >599: outside the protocol space.
no valid HTTP client can produce or interpret them.

---

## why error pages are configured per-code

an operator wants custom HTML for 404 (not found) or 500 (internal
server error), but not 301 (redirect — client is sent elsewhere
immediately, no page rendered). the config maps any valid code to
a file path. the validator rejects codes outside [100, 599] as
unreachable by any conformant client.

---

## history and culture

RFC 2324 (1998) — Hyper Text Coffee Pot Control Protocol — assigned
418 ("I'm a teapot") as an April fools' RFC. formally published,
formally numbered, permanently reserved. google.com returns 418 on
a BREW request. the IETF declined to reclaim it in 2017 after
community protest. 418 is a minor sacred object in programmer
culture — t-shirts, mugs, error pages.
