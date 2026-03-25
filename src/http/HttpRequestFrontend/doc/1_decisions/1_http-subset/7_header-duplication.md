## header duplication: representation and handling


---


### the question

HTTP permits multiple header lines with the same field-name.
how should the parser represent these in `HttpRequest::headers`?


---


### RFC semantics

RFC 9110 §5.3 defines the equivalence rule:

```
a recipient MAY combine multiple field lines with the same name
into one field line, without changing the semantics of the message,
by appending each subsequent field value to the combined field value
in order, separated by a comma...
```
source: https://www.rfc-editor.org/rfc/rfc9110.html#name-field-order



2 field-lines:
```
Accept: text/html
Accept: application/json
```

are semantically equivalent to 1:
```
Accept: text/html, application/json
```


the combined form is not a lossy compression — it is the canonical representation.
the RFC defines the field-value as the comma-joined string.
multiple field-lines are a transport convenience, not distinct semantic entities.

terminology (RFC 9112 §5):
```
field-line  = field-name ":" OWS field-value OWS
```

- `field-line`: one complete header line
- `field-name`: the name portion, left of colon
- `field-value`: the value portion, right of colon

"multiple field lines with the same field name" means:
multiple `field-line` entries sharing a `field-name`,
each with its own `field-value`.
the RFC permits collapsing these into a single `field-line`
whose `field-value` is the comma-concatenation.


---


### type analysis

3 possible candidate types for `headers`:

`std::map<std::string, std::string>` with comma-concat insertion:
stores `field-name → field-value` where `field-value` may contain
commas. matches RFC semantics directly. consumers access
`headers["accept"]` and receive `"text/html, application/json"` —
exactly what the protocol defines as the field-value.

`std::multimap<std::string, std::string>`:
stores each field-line separately. `equal_range()` returns iterator
pair over all values for a key. internal representation diverges from
RFC semantics — every consumer must re-combine values, duplicating the
RFC's specified operation. useful for preserving transport form; not
useful when transport form has no semantic meaning.

`std::vector<std::pair<std::string, std::string>>`:
preserves exact order of all field-lines. no key-based lookup —
requires linear scan. same divergence problem as multimap: consumers
must aggregate. preserves information the RFC declares irrelevant.

the RFC's equivalence rule determines the answer: since multiple
field-lines are semantically identical to one comma-joined field-line,
the internal representation should store the semantic form, not the
transport artefact. `std::map` with comma-concat achieves this.


---


### special cases

`Content-Length`:
RFC 9110 §8.6 specifies distinct handling. multiple `Content-Length`
headers with differing values indicate a smuggling attempt or
malformed message — reject with 400. same value appearing twice:
acceptable, collapse to one instance.

logic (pseudocode):
```
if name is "content-length" and key already exists:
    if existing value ≠ new value:
        reject 400
    else:
        ignore duplicate, keep first
```

implementation: `HttpRequestFrontend_3_headers.cpp`, within
`parse_header_line()`, prior to the general comma-concat path.



`Set-Cookie`:
RFC 6265 defines `Set-Cookie` as an exception to comma-concatenation —
each instance must be preserved separately. however, `Set-Cookie` is a
response header. this parser handles requests; the exception does not apply here.


---


### the decision

`std::map<std::string, std::string>` with comma-concatenation insertion.

`Content-Length` handled separately: reject differing duplicates,
collapse identical duplicates.

implementation: `HttpRequestFrontend_3_headers.cpp`, `parse_header_line()`.


---


### the principle

internal representation should match protocol semantics,
not preserve transport artefacts.

the RFC defines the field-value as the comma-joined aggregate.
storing that aggregate directly means every downstream consumer
operates on the canonical form without transformation.

preserving multiple separate entries (multimap, vector<pair>) would
store information the protocol declares semantically meaningless,
while requiring every consumer to reconstruct the meaningful form.
