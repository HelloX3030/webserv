## interface with src code documentation

src/http/HttpRequestFrontend/doc/

| `meta/2_knowledge/network-protocols/http/` | `src/http/HttpRequestFrontend/doc/` |
|-------------------------------------------|-------------------------------------|
| what HTTP is | how webserv parses HTTP |
| general domain knowledge | implementation-specific decisions |
| reusable across projects | specific to this codebase |
| ontology, semantics | mechanics, code structure |
| RFC-derived understanding | webserv subset + constraints |

---

## future topics (ghr)

domains & concepts to return to for deeper HTTP knowledge:

### protocol mechanics
- chunked transfer encoding (Transfer-Encoding: chunked)
- content negotiation (Accept, Accept-Language, Accept-Encoding)
- conditional requests (If-Match, If-None-Match, If-Modified-Since, ETag)
- range requests (Range, Content-Range, 206 Partial Content)
- compression (Content-Encoding: gzip, deflate, br)
- trailers (Trailer header, chunked encoding trailers)

### caching
- cache-control directives (max-age, no-cache, no-store, private, public)
- validation (ETag, Last-Modified, 304 Not Modified)
- cache hierarchy (browser, proxy, CDN, origin)
- cache invalidation strategies

### security
- TLS/HTTPS layer interaction
- CORS (Cross-Origin Resource Sharing)
- CSP (Content-Security-Policy)
- HSTS (Strict-Transport-Security)
- cookie security attributes (Secure, HttpOnly, SameSite)
- authentication schemes (Basic, Digest, Bearer, OAuth flows)
- request smuggling attacks
- header injection vulnerabilities

### connection management
- HTTP/2 multiplexing, streams, frames
- HTTP/3 and QUIC
- WebSocket upgrade handshake
- server-sent events (SSE)
- connection coalescing

### advanced semantics
- PATCH method and partial updates
- OPTIONS and preflight requests
- CONNECT method (tunnelling)
- 1xx informational responses (100 Continue, 103 Early Hints)
- redirects in depth (301 vs 302 vs 307 vs 308)
- content-disposition and file downloads

### operational
- HTTP observability (access logs, tracing, metrics)
- load balancing and reverse proxies
- HTTP in service mesh architectures
- API gateway patterns

### formal and security research
- HTTP desync attacks (Portswigger research)
- parser differentials and ambiguity exploitation
- RFC compliance testing methodologies
- formal grammar verification
