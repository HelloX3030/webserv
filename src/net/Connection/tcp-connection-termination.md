# TCP connection termination — FIN and RST

## FIN (finish)

orderly close.

sender says: "I'm done sending data. acknowledge this."

four-way handshake:
1. client sends FIN
2. server ACKs
3. server sends FIN (when ready)
4. client ACKs

at application layer: `recv()` returns 0.
meaning: peer closed their write side. no more data coming.

connection half-closed after first FIN.
peer can still send until they also FIN.

---

## RST (reset)

abrupt termination.

no handshake. immediate.
kernel discards send/receive buffers.
connection is dead.

at application layer:
- `recv()` returns -1, errno ECONNRESET
- `send()` returns -1, errno ECONNRESET or EPIPE

causes:
- peer crashed
- peer's kernel timed out the connection
- firewall sent RST
- application called close() with unsent data in buffer (SO_LINGER)
- SYN to closed port

---

## in webserv

detect via return value of recv()/send().

`recv() == 0` → FIN received → close connection.
`recv() == -1` → check errno, likely RST or other error → close connection.
