# URI form

## the question

HTTP defines 4 request-target forms (RFC 9112 §3.2).
which does the server accept?


---


## the 4 forms
```abnf
request-target = origin-form
               / absolute-form
               / authority-form
               / asterisk-form

origin-form    = absolute-path [ "?" query ]   ; /path?query
absolute-form  = absolute-URI                  ; http://host/path
authority-form = uri-host ":" port             ; host:port  (CONNECT)
asterisk-form  = "*"                           ; OPTIONS *
```

origin-form is the normal case: client sends path, server
already knows its own host. absolute-form is used with proxies.
authority-form is exclusive to CONNECT (tunnelling). asterisk-form
is exclusive to OPTIONS server-wide queries.


---


## the decision

accept origin-form only. all other forms: 400 Bad Request.


---


## why

webserv is an origin server, not a proxy. RFC 9112 §3.2.1:
origin servers receiving absolute-form should treat the
URI identically to origin-form after stripping scheme and host.
permitting absolute-form adds parsing complexity for no gain
in this context.

authority-form is tied to CONNECT, which is a tunnelling method.
webserv does not implement CONNECT.

asterisk-form is tied to server-wide OPTIONS. not required by
the subject.

the subset is not arbitrary — it is the exact set of forms
an origin server receiving direct (non-proxy) connections
will encounter in practice.


---


## validation

origin-form is identified by the URI beginning with `/`.
this is a necessary condition: all other forms begin with
a scheme (`http://`), a host, or `*`.

checked in `parse_request_line()` immediately after URI
extraction: `uri[0] != '/'` → 400.

URI character validation (pchar, pct-encoded, sub-delims)
is not performed at this layer. the raw string is stored;
the executor is responsible for path resolution and
any further URI interpretation.
