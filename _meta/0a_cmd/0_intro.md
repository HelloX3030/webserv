# Webserv

overview, description

## context
42
Core Curriculum
Rank05

## format
group project
max. 3 people

## version
24.0

## domains, keywords
Network
Object-oriented programming
Unix
Network & system administration

## summary
write your own HTTP server and test it with a browser.

## introduction

the World Wide Web is a distributed, collaborative, hypermedia
information system. these 3 words capture what made it revolutionary:

- distributed: resources live across millions of machines worldwide.
  no central authority owns or controls the system.
- collaborative: anyone can publish, anyone can link. the web grows
  through participation, not permission.
- hypermedia: documents contain references to other documents.
  a user follows links, navigating a web of interconnected resources.

HTTP (Hypertext Transfer Protocol) is the language that enables this.
it defines how clients request resources and how servers respond.

the protocol emerged from Tim Berners-Lee's work at CERN (1989-1991),
designed to let physicists share research documents across networks.
HTTP/0.9 was minimal: a single method (GET), no headers, plain text.
HTTP/1.0 (1996) introduced headers, status codes, content types.
HTTP/1.1 (1997, RFC 2068; revised 1999, RFC 2616) added persistent
connections, chunked transfer, virtual hosting—the version that
carried the web through its explosive growth.

HTTP became infrastructure. every browser, every API, every connected
device speaks it. to build a web server is to implement the protocol
that underlies modern networked computation.

this project: implement a web server conforming to HTTP/1.1, handling
multiple simultaneous connections, serving static and dynamic content,
managing errors—from raw sockets to complete responses.
