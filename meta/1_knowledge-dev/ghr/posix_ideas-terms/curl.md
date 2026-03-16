# curl


## what it is

command-line tool for transferring data using URLs.

created by Daniel Stenberg, 1998. name: "Client URL" or "see URL".

essentially: an HTTP client you invoke from terminal instead of browser.


## what it does

constructs and sends HTTP requests, displays responses.

```bash
curl http://localhost:8080/
```

this:
1. resolves `localhost` to IP (127.0.0.1)
2. opens TCP connection to port 8080
3. sends HTTP request: `GET / HTTP/1.1` + headers
4. reads response
5. prints response body to stdout

curl handles the protocol. you specify *what* you want;
curl figures out *how* to ask for it.


## relation to network stack

```
┌─────────────────────────────────────┐
│            curl (application)       │  ← you interact here
├─────────────────────────────────────┤
│         HTTP protocol logic         │  ← curl constructs this
├─────────────────────────────────────┤
│      libcurl / socket API           │  ← socket(), connect(), send(), recv()
├─────────────────────────────────────┤
│              TCP                    │  ← reliable byte stream
├─────────────────────────────────────┤
│              IP                     │  ← routing between hosts
├─────────────────────────────────────┤
│     network interface (ethernet)    │  ← physical transmission
└─────────────────────────────────────┘
```

curl uses TCP sockets (via libcurl). it calls the same socket API
your webserv uses: `socket()`, `connect()`, `send()`, `recv()`.

difference: curl is the client side. webserv is the server side.
curl initiates. webserv responds.


## protocols supported

HTTP, HTTPS, FTP, SFTP, SCP, LDAP, MQTT, and ~25 others.

for webserv testing: only HTTP matters.


## usage for webserv testing


### basic GET

```bash
curl http://localhost:8080/
```

sends:
```
GET / HTTP/1.1
Host: localhost:8080
User-Agent: curl/7.x
Accept: */*

```


### verbose mode (-v)

```bash
curl -v http://localhost:8080/
```

shows:
- connection details (IP, port)
- request headers sent (prefixed `>`)
- response headers received (prefixed `<`)
- response body

essential for debugging. see exactly what your server receives
and what it sends back.


### show headers only (-I)

```bash
curl -I http://localhost:8080/
```

sends HEAD request. server returns headers without body.
useful for checking Content-Type, Content-Length.


### include headers in output (-i)

```bash
curl -i http://localhost:8080/
```

GET request, but prints response headers before body.


### POST with data (-d)

```bash
curl -d "name=john&age=30" http://localhost:8080/submit
```

sends:
```
POST /submit HTTP/1.1
Host: localhost:8080
Content-Type: application/x-www-form-urlencoded
Content-Length: 18

name=john&age=30
```


### POST with file upload (-F)

```bash
curl -F "file=@photo.jpg" http://localhost:8080/upload
```

sends multipart/form-data. this is how browsers upload files.


### custom method (-X)

```bash
curl -X DELETE http://localhost:8080/resource/123
```


### custom headers (-H)

```bash
curl -H "Content-Type: application/json" \
     -d '{"key":"value"}' \
     http://localhost:8080/api
```


### follow redirects (-L)

```bash
curl -L http://localhost:8080/old-path
```

if server returns 301/302, curl follows Location header.


### specify HTTP version

```bash
curl --http1.0 http://localhost:8080/
```

forces HTTP/1.0 (what webserv implements). default is HTTP/1.1.


## output interpretation

```
* Trying 127.0.0.1:8080...           # connection attempt
* Connected to localhost             # TCP handshake complete
> GET / HTTP/1.1                     # request line sent
> Host: localhost:8080               # request headers
> User-Agent: curl/7.81.0
> Accept: */*
>                                    # blank line = headers done
< HTTP/1.1 200 OK                    # response status
< Content-Type: text/html            # response headers
< Content-Length: 1234
<                                    # blank line
<!DOCTYPE html>...                   # response body
* Connection #0 left intact          # connection state
```

`>` = sent to server (request)
`<` = received from server (response)
`*` = curl internal info


## why curl, not browser?

- scriptable (automate tests)
- shows raw HTTP (browser hides protocol details)
- precise control (method, headers, body)
- no rendering (see exactly what server sends)
- reproducible (same command = same request)

browser adds: cookies, caching, JavaScript, redirects, prefetch.
curl sends exactly what you tell it.


## comparison with telnet

curl: knows HTTP. constructs valid requests for you.
telnet: knows nothing. sends raw bytes you type.

curl for: "does my server handle POST correctly?"
telnet for: "what happens if I send malformed garbage?"

both use TCP underneath. curl adds HTTP layer on top.