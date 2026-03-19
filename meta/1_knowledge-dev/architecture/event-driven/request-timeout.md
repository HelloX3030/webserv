# request timeout — preventing indefinite hangs

## specification

no client request may hang indefinitely.

---

## failure mode

client connects, sends partial HTTP request, goes silent.

server received `GET /index` — waiting for terminator (`\r\n\r\n`).
no more bytes arrive.
`poll()` never fires again on this fd.

connection sits open.
fd occupied.
memory allocated for buffers and connection state.

multiply by enough misbehaving or crashed clients → fd exhaustion.

this is not a polling failure - polling works correctly — it wakes when data arrives.
the problem: data never arrives.

---

## mechanism

`poll()`/`epoll_wait()` accepts a `timeout_ms` parameter.

when it returns with no events (timeout fired):
  sweep all active connections.
  check each connection's last-activity timestamp.
  any connection idle beyond threshold → close it, free fd.

no signals needed.
timeout handled entirely within event loop via `timeout_ms`
and a timestamp per connection.

---

## timeout value

nginx, Apache: 60 seconds default for request receipt.

exact value matters less than having the mechanism.
this is resource hygiene, not performance tuning.

---

## on timeout expiry

close the connection.

optionally: send `408 Request Timeout` before closing.
practically: the client went silent — it's likely not listening.
sending response to dead connection wastes cycles.

check subject requirements. if error responses required for all codes,
implement 408. otherwise, just `close(fd)`.
