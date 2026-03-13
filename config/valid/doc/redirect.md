## redirect.conf

HTTP redirection: server responds with 3xx status and Location header;
client automatically requests the new URI.

demonstrates: return directive (status code + target path), the
non-terminal nature of redirect responses.


## mechanism

request for /old/anything:
1. server matches location /old/
2. location has return_code set → skip file resolution
3. server emits:
```
   HTTP/1.1 301 Moved Permanently
   Location: /new/anything
   Content-Length: 0
```
4. client receives redirect, automatically requests /new/anything
5. server matches location /new/, serves www/html/anything

no file exists for /old/. no error page involved. the redirect response
has no body — the Location header is the entire payload.


## status codes

the return directive accepts codes in [300, 399]. common choices:

| code | name | use |
|------|------|-----|
| 301 | Moved Permanently | resource has moved forever; clients/search engines update references |
| 302 | Found | temporary redirect; client should keep using original URI |
| 307 | Temporary Redirect | like 302 but preserves HTTP method (POST stays POST) |
| 308 | Permanent Redirect | like 301 but preserves HTTP method |

for webserv evaluation, 301 is sufficient. the server's job is identical
for all codes — emit the configured number; the client interprets it.


## what this deliberately omits

. error_page — redirects are not errors; no error page involved
. cgi — separate scenario
. uploads — separate scenario


## path dependencies

. www/html/index.html — the file served after redirect completes


## to test
```
curl -v http://127.0.0.1:8080/old/
```

expected: response with status 301, Location: /new/. curl does not
follow redirects by default; add -L to follow:
```
curl -L http://127.0.0.1:8080/old/
```

expected: final response is the content from /new/ (www/html/index.html).
