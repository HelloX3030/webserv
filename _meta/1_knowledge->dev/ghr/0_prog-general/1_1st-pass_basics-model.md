# webserv — what is this?


## situation

you type a URL into your browser & a webpage appears.

```
    You                           Somewhere else
    
   [Browser]  ───── request ─────►  [    ?    ]
              ◄──── response ────   [         ]
```

the `?` is a server — what we're building.


## what a server does

a program that:

1. waits — sits idle until contacted
2. listens — hears incoming requests
3. responds — sends back what was asked for

a responder: doesn't initiate, doesn't seek.
waits to be asked, then answers.


## the messenger model

```
                  ┌─────────────────────────────────────┐
                  │             YOUR SERVER             │
                  │                                     │
  [Browser] ◄────►│  network  ◄──►  logic  ◄──►  disk   │
                  │                                     │
                  └─────────────────────────────────────┘
```

your server is a messenger between:

- the network — where requests arrive
- the disk — where files live

browser can't touch disk. disk knows nothing about networks.
server bridges them.


## what flows through

requests and responses — text messages following HTTP format.


request (browser → server):
```
GET /hello.html HTTP/1.0
Host: example.com

```
"give me /hello.html"


response (server → browser):
```
HTTP/1.0 200 OK
Content-Type: text/html

<html><body>Hello!</body></html>
```
"here it is, it's HTML"


HTTP is structured text. 
server reads, understands, fetches, sends.


---


## core challenge

1 server, many browsers, simultaneously.

```
  Browser A ───►  ┌────────┐
  Browser B ───►  │ SERVER │
  Browser C ───►  └────────┘
       ⋮
```

some fast, some slow, some disconnect mid-conversation,
some send garbage.

handle all at once without:
- freezing on slow clients
- crashing on bad input
- losing track of who asked what

this is the hard part: not HTTP — concurrency.


--- 


## 2 phases

### startup (once)

read config → understand rules:
- which port?
- where do files live?
- what URLs map where?

config then frozen.

### runtime (forever)

```
loop:
    wait for something to happen
    figure out what
    do next small step
    repeat
```

loop never ends, until you kill the server.


---


## "something happens"

you ask the OS:

> "any connections ready for attention?"


OS answers:

> "client #7 sent bytes. client #12 ready for you to send."


handle those two, ask again. forever.

this is I/O multiplexing: 1 program, many conversations, no paralysis.


---


## shape of a conversation

each client goes through stages:

```
accept connection
    ↓
read bytes (maybe multiple reads)
    ↓
parse: understand what they want
    ↓
process: find file / run script / reject
    ↓
write bytes (maybe multiple writes)
    ↓
close
```

each client in one stage at any moment.
server tracks them all.


---


## configuration

rules the server follows:

```
server {
    listen 8080;
    
    location / {
        root /var/www;
    }
}
```

- listen on port 8080
- `/something` → look in `/var/www/something`

config defines mapping between URLs and reality.


---


## summary

```
┌────────────────────────────────────────────────────────────┐
│                                                            │
│   CONFIG (rules)                                           │
│      │                                                     │
│      ▼                                                     │
│   SERVER (messenger)                                       │
│      │                                                     │
│      ├───► listen for connections                          │
│      │                                                     │
│      └───► for each connection:                            │
│               read request                                 │
│               understand it                                │
│               find what they want                          │
│               send response                                │
│               close                                        │
│                                                            │
│   loop forever, handling whoever is ready                  │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

a webserver:

- messenger between network and disk
- speaks HTTP
- juggles many conversations
- follows config rules