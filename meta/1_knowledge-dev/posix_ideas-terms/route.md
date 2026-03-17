# routing — within a web server

## disambiguation

2 different processes, 1 word:

```
networking:   forwarding packets across the internet toward
              destination IP, hop by hop, via routing tables.

web server:   given an incoming HTTP request, determine which
              configured location block handles it.
```

this doc is about the second ctx: within a web server.

---

## mechanics

client sends:

```
POST /upload/image.jpg HTTP/1.1
Host: example.com
```

after `accept()` + HTTP parse, server has:

```
method: POST
uri:    /upload/image.jpg
host:   example.com
```

routing = matching `uri` against configured location blocks.

config:

```
location / {
    root /var/www/html;
}

location /upload/ {
    root /var/uploads;
    upload_enable on;
    upload_store /var/uploads;
}
```

`/upload/` matches `/upload/image.jpg` and is more specific than `/`.
∴ request handled by `/upload/` location.

---

## strategy — longest prefix match

among all location blocks whose path is a prefix of the request URI,
select the longest. deterministic: more specific rules win.

alternative strategies (exact match, regex match) exist but are not
implemented here.

---

## HTTP methods — the common misconception

POST ≠ instruction to send data outward.
POST = client declaring: "i am sending data to you."

```
GET    — give me this resource
POST   — here is data; process it
PUT    — store this at this URI
DELETE — remove this resource
```

server receives POST, acts locally per configuration.
it does not forward anything to the internet unless explicitly
configured as proxy or API gateway — a separate, distinct action.

the location directive is a URI path pattern, not a domain.
the domain was resolved by DNS before the TCP connection arrived.
by the time routing executes, only the URI path remains.

---

## position in the pipeline

```
accept()     — OS hands us a connected socket
read()       — receive raw bytes
HTTP parse   — bytes → request struct (method, uri, headers, body)
routing      — uri → Location config
handler      — Location config + request → response
write()      — send response bytes
close()      — or keep-alive
```

routing is the hinge: generic network layer → application layer.
where the server's configuration becomes active.
