Method handlers: What to do when receiving GET/POST/DELETE.
behaviour.

Handlers use HTTP types but aren't about HTTP. They're about file serving, CGI execution, redirects. Different rate of change.



naming is asymmetric.
The predicate is "application behaviour triggered by HTTP methods" (file serving, CGI, redirects).
The name describes a role, not a domain.
The role-based naming reflects reality: this layer is about processing, not domain. It's where "what to do" lives.

Compare: http/ (protocol name), config/ (concern name), handlers/ (role name).
This is honest but stylistically inconsistent.

The asymmetry isn't necessarily wrong — it reflects the fact that handlers/ is categorically different from http/ or config/.
Those are about something (a protocol, a concern). handlers/ does something.
