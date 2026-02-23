# telnet


## what it is

raw TCP connection tool.

originally (1969): protocol for remote terminal access. connect to
remote machine, type commands, see output. port 23.

now: mostly used as debugging tool. connect to any TCP port,
send arbitrary bytes, see what comes back.


## what it does

opens TCP connection. that's it.

no protocol awareness. no HTTP knowledge. no formatting.
whatever you type goes to the server as raw bytes.
whatever server sends back appears on your screen as raw bytes.

```bash
telnet localhost 8080
```

this:
1. resolves `localhost` to 127.0.0.1
2. opens TCP connection to port 8080
3. waits for you to type
4. sends your keystrokes as bytes
5. displays server's response bytes


## relation to network stack

```
┌─────────────────────────────────────┐
│         telnet (application)        │  ← you type raw bytes
├─────────────────────────────────────┤
│         (no protocol logic)         │  ← nothing here
├─────────────────────────────────────┤
│            socket API               │  ← socket(), connect(), send(), recv()
├─────────────────────────────────────┤
│              TCP                    │  ← reliable byte stream
├─────────────────────────────────────┤
│              IP                     │  ← routing
├─────────────────────────────────────┤
│        network interface            │  ← physical
└─────────────────────────────────────┘
```

telnet is a thin wrapper over TCP sockets. no application protocol.
you *are* the application protocol — you type the bytes.


## why telnet for webserv testing

you see exactly what HTTP looks like on the wire.

no abstractions. no help. if you mistype, server sees your mistake.
this is how you understand the protocol at byte level.


## usage


### connect

```bash
telnet localhost 8080
```

output:
```
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
```

cursor waits. TCP connection open. server waiting for your request.


### send HTTP request

type exactly:
```
GET / HTTP/1.0
Host: localhost

```

note: blank line at end (press Enter twice). this signals end of
headers. without it, server waits forever for more headers.

server responds:
```
HTTP/1.0 200 OK
Content-Type: text/html
Content-Length: 43

<html><body>Hello World</body></html>
Connection closed by foreign host.
```

connection closes (HTTP/1.0 default behavior).


### what you're actually sending

when you type `GET / HTTP/1.0` and press Enter:
```
G E T   /   H T T P / 1 . 0 \r \n
```

`\r\n` = CRLF = carriage return + line feed. HTTP line terminator.

blank line = `\r\n\r\n` = headers complete.


### POST request

```bash
telnet localhost 8080
```

type:
```
POST /submit HTTP/1.0
Host: localhost
Content-Type: application/x-www-form-urlencoded
Content-Length: 18

name=john&age=30
```

Content-Length must match body length exactly.
server reads exactly that many bytes as body.


### testing malformed requests

this is where telnet shines. send garbage:

```
INVALID REQUEST
```

see how your server responds. should return 400 Bad Request.

```
GET /HTTP/1.0
```

missing space. malformed. your parser should reject.

```
GET / HTTP/1.0
Header-Without-Value
```

malformed header. should reject.


## interpreting results


### success

```
HTTP/1.0 200 OK
...body...
Connection closed by foreign host.
```

server parsed request, found resource, sent response, closed.


### error responses

```
HTTP/1.0 400 Bad Request
```

your request was malformed. server rejected it.

```
HTTP/1.0 404 Not Found
```

valid request, but resource doesn't exist.


### no response

cursor hangs. server didn't respond. possibilities:
- server waiting for more data (forgot blank line?)
- server blocked/crashed
- server didn't understand request, waiting for timeout


### connection refused

```
telnet: Unable to connect to remote host: Connection refused
```

nothing listening on that port. server not running.


## telnet vs nc (netcat)

both do raw TCP. nc more flexible:

```bash
# send request from file
cat request.txt | nc localhost 8080

# listen mode (act as server)
nc -l 8080
```

telnet: interactive typing.
nc: scriptable, can pipe input.

for manual debugging: telnet.
for scripted tests: nc.


## telnet vs curl

telnet: you construct HTTP manually. full control. no help.
curl: constructs HTTP for you. convenient. hides details.

```
telnet                          curl
──────                          ────
raw TCP                         HTTP client
you type protocol               tool knows protocol
see exact bytes                 see formatted output
test malformed input            test valid requests
learn protocol                  use protocol
```

use telnet to understand HTTP.
use curl to test your server efficiently.
use both during development.


## 42 subject requirement

> "your server must never block and client can be bounced properly if
> necessary. non-blocking file descriptors must be used...
> you must be able to test with telnet and curl."

telnet tests:
- basic GET works
- POST with body works  
- malformed requests return 400
- server doesn't hang on slow/partial input
- server closes connection appropriately

the subject explicitly requires telnet testing because it exposes
the raw protocol. no browser abstractions hiding bugs.