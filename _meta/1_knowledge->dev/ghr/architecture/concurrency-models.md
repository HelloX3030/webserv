# concurrency models

server concurrency: how a server handles multiple clients simultaneously.

a server exists to serve requests, and requests arrive from multiple clients,
overlapping in time. the server must not block 1 client while serving another.


---


## the ontological situation

```
client_1  ──request──▶  ┌────────┐
client_2  ──request──▶  │ server │  ──response──▶  client_1
client_3  ──request──▶  └────────┘  ──response──▶  client_2
     ⋮                                    ⋮
```

simultaneity is the condition — clients do not coordinate, so the server must.


---


## why concurrency is necessary

I/O is slow relative to CPU: reading from a socket takes microseconds to
milliseconds, while executing application logic takes nanoseconds.

a sequential server — handling 1 request fully before the next — wastes
CPU cycles waiting for I/O. clients queue behind slow operations and
latency compounds.

concurrency allows the server to serve client_2 while waiting for client_1's
data, overlapping I/O waits with useful work and utilising hardware more fully.


---


## the models — taxonomy

3 fundamental approaches, each answering differently: 
where does the concurrency live?


### 1. process-per-connection

```
                    ┌─────────────┐
client_1  ────────▶ │  process_1  │
                    └─────────────┘
                    ┌─────────────┐
client_2  ────────▶ │  process_2  │
                    └─────────────┘
```

mechanism:
    the main process accepts a connection, then fork() creates a child
    process to handle the entire request lifecycle while the parent
    continues accepting.

properties:
    strong isolation — a process crash affects only that client.
    simplicity — each process runs sequential code.
    overhead — process creation is expensive (memory, kernel structures).
    scaling — limited by OS process limits and memory.

historical context:
    Apache 1.x prefork model; CGI, where each request spawns a process.

appropriate when:
    isolation is paramount (untrusted code execution), request rate is low,
    or simplicity outweighs efficiency.


### 2. thread-per-connection

```
                    ┌─────────────┐
                    │   process   │
                    │  ┌───────┐  │
client_1  ────────▶ │  │ thr_1 │  │
                    │  └───────┘  │
                    │  ┌───────┐  │
client_2  ────────▶ │  │ thr_2 │  │
                    │  └───────┘  │
                    └─────────────┘
```

mechanism:
    a single process with each connection handled by a dedicated thread;
    threads share address space.

properties:
    lower overhead than processes (shared memory, faster creation).
    concurrency within a single address space.
    shared state requires synchronisation (mutexes, atomics).
    scaling limited by thread stack memory and context switching costs.
    complexity from race conditions and deadlocks.

historical context:
    Java servlet containers (Tomcat thread pool), traditional database servers.

appropriate when:
    connection counts are moderate (100s to low 1000s), shared state is
    manageable, and blocking I/O is acceptable.


### 3. event-driven (single-threaded multiplexing)

```
                    ┌─────────────────────────┐
                    │        process          │
                    │                         │
                    │  ┌───────────────────┐  │
                    │  │    event loop     │  │
client_1  ─────────▶│  │                   │  │
client_2  ─────────▶│  │  poll/epoll/kqueue│  │
client_3  ─────────▶│  │        ↓          │  │
                    │  │  dispatch handler │  │
                    │  └───────────────────┘  │
                    └─────────────────────────┘
```

mechanism:
    a single process and single thread using an I/O multiplexing syscall
    (select, poll, epoll, kqueue). the kernel reports which fds are ready,
    and the application dispatches to appropriate handlers. handlers must
    not block — they return control quickly.

properties:
    minimal overhead per connection (no thread/process per client).
    scales to 10000s of connections (the C10K problem).
    no shared-state races (single thread).
    complexity from callback-based or state-machine code.
    constraint: handlers must be non-blocking.

historical context:
    nginx (2004), Node.js (2009), Redis.

appropriate when:
    connection counts are high, workloads are I/O-bound, and blocking
    operations can be avoided or offloaded.

webserv uses this model — the epoll-based event loop is the architectural
centre.


---


## trade-off structure

```
                    process-per-conn    thread-per-conn    event-driven
                    ────────────────    ───────────────    ────────────
isolation           strong              weak               none
overhead/conn       high                medium             minimal
max connections     ~100s               ~1000s             ~10000s
code complexity     low                 medium             high
shared state        none                requires sync      none (single thread)
blocking I/O        natural             natural            forbidden
```


---


## hybrid models

the models combine in practice.

thread pool + event loop:
    an event loop dispatches to worker threads for CPU-bound tasks —
    nginx with thread pools for disk I/O exemplifies this.

multi-process + event-driven:
    multiple processes each running an event loop, as in the nginx
    master + worker architecture.

async/await (language-level):
    cooperative multitasking on an event loop — Rust tokio, Python asyncio,
    JavaScript. syntactic sugar over callbacks and state machines.


---


## language perspectives

### Haskell

lightweight threads (green threads) managed by the runtime. `forkIO` spawns
a thread with roughly 1KB stack, and the runtime multiplexes onto OS threads.
the I/O manager uses epoll/kqueue internally, so the programmer writes
sequential code while the runtime handles multiplexing.

```haskell
main = do
    sock <- listenOn port
    forever $ do
        (conn, _) <- accept sock
        forkIO $ handleClient conn   -- lightweight, 1000s possible
```

the event-driven complexity hides beneath a sequential abstraction.


### Rust

explicit choice via async runtimes. tokio provides event-driven execution
with a work-stealing thread pool. `async`/`await` syntax enables cooperative
tasks, but there is no hidden runtime — the programmer controls the executor.

```rust
#[tokio::main]
async fn main() {
    let listener = TcpListener::bind("0.0.0.0:8080").await.unwrap();
    loop {
        let (socket, _) = listener.accept().await.unwrap();
        tokio::spawn(async move {
            handle_client(socket).await;
        });
    }
}
```

explicit async with no garbage collection and zero-cost abstractions.


### Agda

not typically used for systems programming. concurrency can be modelled
via coinduction, interaction trees, or the IO monad. formal verification
of concurrent protocols is possible, but the concern is correctness proofs
rather than runtime efficiency.


---


## further reading

Kegel, D. "The C10K problem." 1999.
    http://www.kegel.com/c10k.html
    the document that named the scaling challenge.

Stevens, W. Richard. Unix Network Programming, Vol. 1. 3rd ed.
    ch. 6: I/O multiplexing — the authoritative treatment.

libevent, libev, libuv documentation.
    event loop libraries abstracting platform differences.