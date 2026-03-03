# runtime — persistence support requirements


## context

persistent connections are a runtime concern.
the HTTP request frontend exposes the data.
the runtime makes the decision and acts on it.

the frontend can be "persistent-ready" before the runtime is.
the frontend doesn't need to know whether persistence is enabled —
it just exposes the data. the runtime makes the decision.


---


## what the runtime must handle


### 1. connection state machine extension

current model (single-request):

```
accept → reading → complete → close
```

persistent model:

```
accept → reading → complete → idle → reading → complete → idle → ...
                                                              ↓
                                                            close
```

the new state: **idle** (waiting for next request).

the fd is registered with poll(), but neither reading nor writing.
the connection object exists, holds state, but is dormant.


### 2. poll() registration for idle connections

idle connections must remain registered with poll() for:
- POLLIN: client sends next request
- POLLHUP / POLLERR: client disconnected

when POLLIN fires on an idle connection:
transition from idle → reading, begin parsing next request.


### 3. connection reset between requests

after a response is written and keep-alive is decided:

```cpp
if (request.keepAlive() && response.completedCleanly())
{
    connection.resetForNextRequest();
    /* fd stays open, stays in poll */
    /* request/response buffers cleared */
    /* state transitions to idle */
}
else
{
    connection.close();
    /* fd closed, removed from poll */
}
```

`resetForNextRequest()` must:
- clear request buffer (or mark as consumed)
- clear response buffer
- reset parser state
- update last_activity timestamp
- transition state to idle


### 4. timeout mechanism for idle connections

a persistent connection without timeout is a resource leak vector.
malicious or buggy clients can hold connections open indefinitely.


#### mechanism

- store `last_activity` timestamp per connection
- on each poll cycle (or periodically):
  - for each idle connection:
    - if (now - last_activity) > timeout: close connection


#### reference values

NGINX: `keepalive_timeout` default 75 seconds
Apache: `KeepAliveTimeout` default 5 seconds

for webserv: 60 seconds is reasonable.
make configurable if time permits.


### 5. request count limit (optional)

NGINX: `keepalive_requests` default 1000
Apache: `MaxKeepAliveRequests` default 100

after N requests on a connection, close it regardless of
keep-alive preference. prevents memory accumulation and
ensures connections eventually cycle.

lower priority than timeout. implement if time permits.


---


## decision logic

the runtime owns this:

```cpp
void Runtime::afterResponseWritten(Connection& conn)
{
    const HttpRequest& req = conn.request();
    const HttpResponse& res = conn.response();

    if (req.keepAlive() && res.completedCleanly())
    {
        conn.resetForNextRequest();
        conn.setState(ConnectionState::IDLE);
        /* fd remains in poll set */
    }
    else
    {
        conn.close();
        /* fd removed from poll set */
    }
}
```


---


## what the runtime does NOT do

- parse headers (frontend's job)
- determine HTTP version (frontend's job)
- decide client preference (frontend exposes via keepAlive())

the runtime's persistence responsibilities:
- make the final decision (combining client preference + response
  status + config)
- execute the decision (keep open or close)
- manage connection state machine
- enforce timeouts