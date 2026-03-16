## predicate

application behaviour. what to do when receiving GET/POST/DELETE.

file serving, CGI dispatch, redirects.
uses HTTP types but isn't *about* HTTP — different rate of change.

---

## naming

"handlers" — role-based, not domain-based.

this layer is about *processing*, not *domain*.
it's where "what to do" lives.

the asymmetry with other categories (http/, config/) is intentional:
those are *about* something; handlers/ *does* something.

---

## v0 → v1

previously nested in `http/handlers/`. extracted to top-level.

separation rationale: handlers depend on http/ types,
but http/ shouldn't depend on handler implementations.
