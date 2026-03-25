# uniform resource identifiers


## the problem

how do you refer to something unambiguously?

in conversation: "the book on the table" — context-dependent, ambiguous.
in a network of millions of machines: context is absent. ambiguity is failure.

identification requires:
1. a syntax everyone agrees on
2. enough structure to be unambiguous
3. independence from any single system or protocol


---


## the solution

a URI (uniform resource identifier) is a compact character sequence
that identifies a resource.

- **uniform**: 1 syntax for all schemes (http, file, mailto, urn, ...)
- **resource**: anything that can be identified — document, service, concept, person
- **identifier**: a name that picks out exactly 1 thing

the telos: unambiguous identification across contexts.


---


## etymology and terminology

**URI**: the general term. any identifier following RFC 3986 syntax.

**URL** (uniform resource locator): a URI that provides access information.
"where it is and how to get it." includes scheme (protocol) and authority (host).
example: `https://example.com/doc.html`

**URN** (uniform resource name): a URI that names without locating.
"what it is called." persistent even if location changes.
example: `urn:isbn:0451450523`

URL and URN are subsets of URI. the distinction is semantic, not syntactic.


---


## generic syntax

RFC 3986 defines a scheme-independent structure:

```
URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
```


5 components:

| component   | role                                      |
|-------------|-------------------------------------------|
| scheme      | names the identifier type or protocol     |
| authority   | naming authority (often host)             |
| path        | hierarchical identification               |
| query       | non-hierarchical parameters               |
| fragment    | secondary resource within the primary     |


---


## decomposition
```
  https://example.com:8080/api/users?active=true#section-3
  \___/   \______________/\________/ \_________/ \_______/
    |            |            |           |          |
  scheme    authority       path       query     fragment
```

**scheme**: `https`
identifies the protocol. determines how to interpret the rest.

**authority**: `example.com:8080`
the naming authority. for network protocols: host and optional port.
syntax: `[ userinfo "@" ] host [ ":" port ]`

**path**: `/api/users`
hierarchical. segments separated by `/`.
identifies a resource within the authority's namespace.

**query**: `active=true`
non-hierarchical. often key-value pairs, but RFC 3986 imposes no structure.
begins after `?`, ends at `#` or end of URI.

**fragment**: `section-3`
identifies a secondary resource within the primary.
client-side only — never transmitted to the server.


---


## character encoding

URIs use a restricted character set (ASCII subset).

**unreserved**: `A-Z a-z 0-9 - . _ ~`
safe anywhere. no encoding required.

**reserved**: `: / ? # [ ] @ ! $ & ' ( ) * + , ; =`
have syntactic meaning. must be percent-encoded if used as data.

**percent-encoding**: `%HH`
represents arbitrary octets. HH is the hex value.
example: space → `%20`, `é` → `%C3%A9` (UTF-8 bytes)


---


## formal grammar (simplified)
```abnf
URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

hier-part     = "//" authority path-abempty
              / path-absolute
              / path-rootless
              / path-empty

authority     = [ userinfo "@" ] host [ ":" port ]
host          = IP-literal / IPv4address / reg-name
port          = *DIGIT

path-abempty  = *( "/" segment )
path-absolute = "/" [ segment-nz *( "/" segment ) ]
segment       = *pchar
segment-nz    = 1*pchar

query         = *( pchar / "/" / "?" )
fragment      = *( pchar / "/" / "?" )

pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
pct-encoded   = "%" HEXDIG HEXDIG
unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
sub-delims    = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
```


---


## key properties

1. **syntax is context-free**
   parseable without knowing the scheme. generic structure first,
   scheme-specific interpretation second.

2. **scheme determines semantics**
   `http:` means network retrieval.
   `file:` means local filesystem.
   `mailto:` means email composition.
   same syntax, different meaning.

3. **hierarchical path**
   segments form a tree. `/a/b/c` implies `/a/b` implies `/a`.
   enables relative resolution.

4. **normalisation**
   equivalent URIs may differ in surface form.
   `HTTP://Example.COM/` and `http://example.com/` identify the same resource.
   normalisation produces a canonical form for comparison.


---


## references

RFC 3986: Uniform Resource Identifier (URI): Generic Syntax
RFC 7230: HTTP/1.1 Message Syntax and Routing (URI in HTTP context — see 1_webserv/)
