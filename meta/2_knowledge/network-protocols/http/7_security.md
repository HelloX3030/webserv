# http security

security mechanisms at and around the HTTP layer.
TLS, E2E encryption, security headers, authentication, attack surfaces.


---


## transport security: TLS

HTTP transmits in cleartext. any observer on the network path
(ISP, WiFi operator, router compromise) can read the content.

TLS (Transport Layer Security) wraps HTTP in encryption:

```
┌─────────────────────────────────────┐
│         HTTP (cleartext)            │
├─────────────────────────────────────┤
│         TLS (encryption)            │
├─────────────────────────────────────┤
│         TCP                         │
└─────────────────────────────────────┘
```

HTTPS = HTTP over TLS. the protocol is identical; the transport differs.

TLS provides:
- **confidentiality** — content encrypted with symmetric cipher
- **integrity** — MAC detects tampering
- **authentication** — server proves identity via certificate

the TLS handshake establishes a shared secret using asymmetric
cryptography (typically ECDHE). subsequent communication uses
symmetric encryption (typically AES-GCM or ChaCha20-Poly1305).


---


## what TLS protects against

network observers cannot read content:
- ISPs see connection metadata (IP addresses, timing) but not HTTP
- WiFi snoopers see encrypted packets
- MITM attackers cannot inject content without detection

certificate validation prevents impersonation:
- the server presents a certificate signed by a trusted CA
- the client verifies the chain to a root CA
- domain name in certificate must match requested host


---


## what TLS does not protect against

**the server itself** — TLS terminates at the server.
the server sees all HTTP content in cleartext.
if the server is compromised, TLS provides no protection.

**intermediaries** — reverse proxies, CDNs, load balancers
often terminate TLS and re-encrypt to origin servers.
at each termination point, HTTP is cleartext.

```
┌────────┐      TLS      ┌────────┐      TLS      ┌────────┐
│ Client │◄────────────► │  CDN   │◄────────────► │ Origin │
└────────┘               └────────┘               └────────┘
                              │
                         cleartext
                         (CDN sees)
```

**application vulnerabilities** — TLS encrypts the transport.
SQL injection, XSS, auth bypass happen at the application layer.
TLS does not prevent them.

**metadata** — TLS encrypts content but not all metadata.
- IP addresses visible (necessary for routing)
- timing patterns visible
- packet sizes visible (can sometimes leak information)
- SNI (Server Name Indication) historically cleartext (ECH addresses this)


---


## end-to-end encryption

E2E encryption protects against intermediaries including the server.

```
┌─────────┐              ┌─────────┐              ┌─────────┐
│  Alice  │────────────► │ Server  │────────────► │   Bob   │
└─────────┘              └─────────┘              └─────────┘
     │                        │                        │
  encrypt                  opaque                   decrypt
 (Alice's key            ciphertext               (Bob's key)
                     (server cannot read)
```

the message is encrypted at the application layer before
entering HTTP. HTTP carries the ciphertext as payload.
TLS encrypts the HTTP (double encryption, different purposes).
the server relays encrypted blobs it cannot decrypt.

examples:
- Signal: servers relay E2E encrypted messages
- ProtonMail: email content encrypted, servers blind
- standard HTTPS banking: *not* E2E — the bank must read your data

E2E requires key management between endpoints.
the server facilitates communication but holds no keys.


---


## layering: TLS vs E2E

```
┌─────────────────────────────────────┐
│  E2E encrypted payload              │  ← only endpoints decrypt
├─────────────────────────────────────┤
│  HTTP message (carries ciphertext)  │  ← server sees structure, not content
├─────────────────────────────────────┤
│  TLS (encrypts HTTP)                │  ← network observers see nothing
├─────────────────────────────────────┤
│  TCP                                │
└─────────────────────────────────────┘
```

TLS protects the *channel*. E2E protects the *content*.
both can be used together. they address different threat models.


---


## security headers

HTTP headers that instruct browsers on security policy.

### HSTS (HTTP Strict Transport Security)

```
Strict-Transport-Security: max-age=31536000; includeSubDomains
```

tells browsers: always use HTTPS for this domain.
prevents downgrade attacks and accidental HTTP requests.
once set, browsers refuse HTTP connections for `max-age` seconds.

### Content-Security-Policy (CSP)

```
Content-Security-Policy: default-src 'self'; script-src 'self' https://trusted.com
```

restricts sources of scripts, styles, images, etc.
primary defence against XSS — even if attacker injects script tag,
browser refuses to execute scripts from disallowed sources.

### X-Frame-Options

```
X-Frame-Options: DENY
```

prevents page from being embedded in frames.
defence against clickjacking attacks.
superseded by CSP `frame-ancestors` but still widely used.

### X-Content-Type-Options

```
X-Content-Type-Options: nosniff
```

prevents browsers from MIME-sniffing responses.
if server says `Content-Type: text/plain`, browser will not
execute it as JavaScript even if content looks like JS.

### referrer-policy

```
Referrer-Policy: strict-origin-when-cross-origin
```

controls what referrer information is sent with requests.
limits information leakage to third parties.


---


## authentication at HTTP level

### Basic authentication

```
Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ=
```

base64-encoded `username:password`. no encryption.
must use HTTPS or credentials are exposed.

### Bearer tokens

```
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

token-based authentication. commonly JWT (JSON Web Token).
server validates token signature and claims.
token is opaque string from HTTP's perspective.

### cookies

```
Set-Cookie: session=abc123; Secure; HttpOnly; SameSite=Strict
```

server sets cookie; browser sends it with subsequent requests.
security attributes:
- `Secure`: only sent over HTTPS
- `HttpOnly`: inaccessible to JavaScript (XSS mitigation)
- `SameSite`: controls cross-origin sending (CSRF mitigation)

cookies are the foundation of session management.
stateless HTTP + stateful cookies = session abstraction.


---


## attack surfaces

understanding HTTP security enables recognising vulnerabilities.

### application logic layer (OSWE focus)

- **SQL injection** — malicious SQL in HTTP parameters
- **XSS** — malicious scripts injected via HTTP
- **CSRF** — forged requests using victim's cookies
- **authentication bypass** — flaws in login/session logic
- **authorisation flaws** — accessing resources without permission
- **deserialization** — malicious objects in HTTP payloads
- **SSRF** — server makes requests to attacker-controlled destinations

these exploit how applications *use* HTTP, not HTTP itself.

### protocol layer

- **request smuggling** — ambiguity between frontend/backend parsing
- **HTTP desync** — exploiting parser differentials
- **header injection** — CRLF injection to add headers
- **cache poisoning** — corrupting cached responses

these exploit ambiguities in HTTP parsing and handling.

### transport layer

- **downgrade attacks** — forcing HTTP instead of HTTPS
- **certificate validation failures** — accepting invalid certs
- **weak cipher suites** — breaking encryption

these exploit TLS configuration or implementation.


---


## references

RFC 9110: HTTP Semantics
    https://www.rfc-editor.org/rfc/rfc9110
    authentication framework in section 11.

RFC 8446: TLS 1.3
    https://www.rfc-editor.org/rfc/rfc8446

OWASP: HTTP Security Response Headers
    https://owasp.org/www-project-secure-headers/

PortSwigger Web Security Academy
    https://portswigger.net/web-security
    free, comprehensive coverage of web vulnerabilities.

James Kettle: HTTP Desync Attacks
    https://portswigger.net/research/http-desync-attacks
    foundational research on request smuggling.
