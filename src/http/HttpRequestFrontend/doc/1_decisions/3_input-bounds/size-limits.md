## limits: where they come from

### the question

what limits apply to URI length, header count, header value size,
body size? where are they specified?

### the analysis

RFC 9112 does not mandate specific limits. it recommends servers
impose limits and respond with appropriate status codes:
- 414 URI Too Long
- 431 Request Header Fields Too Large
- 413 Content Too Large

webserv operates within a configuration context. limits should derive
from `ServerConfig` or have sensible compile-time defaults.

body size: `client_max_body_size` from config.
URI length: no config option currently. compile-time default.
header size: no config option currently. compile-time default.

### the decision

- body size: from `ServerConfig::client_max_body_size` (or location override)
- URI length: compile-time constant, e.g. 8192 bytes
- header line length: compile-time constant, e.g. 8192 bytes
- header count: compile-time constant, e.g. 100 headers
- total headers size: compile-time constant, e.g. 32768 bytes

the frontend receives the relevant limit (body size) from the caller.
other limits are internal constants.

### the principle

limits exist to bound resource consumption.
configurable limits: operator controls resource allocation.
fixed limits: prevent pathological input, reasonable defaults.
