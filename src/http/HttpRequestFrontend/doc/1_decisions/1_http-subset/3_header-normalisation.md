## header normalisation: lowercase

### the question

HTTP header field names are case-insensitive (RFC 9110 section 5.1).
should the parser store them as-received or normalise?

### the analysis

options:
1. store as-received, case-insensitive lookup
2. normalise to lowercase, exact-match lookup

option 1 preserves original form but complicates every lookup.
option 2 loses original form but simplifies all subsequent access.

the original case has no semantic meaning — `Content-Length`,
`content-length`, `CONTENT-LENGTH` are identical per spec.
preserving meaningless variation creates work without value.

### the decision

normalise to lowercase during header parsing.
lookup uses exact match on lowercase key.

```cpp
std::transform(name.begin(), name.end(), name.begin(), ::tolower);
headers_[name] = value;
```

### the principle

normalise at the boundary.
canonical form internally, conversion once at entry.
