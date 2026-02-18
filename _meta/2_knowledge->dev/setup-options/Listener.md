## issue: Listener class has no clear purpose
```cpp
// Current (from codebase scan)
class Listener {
    int fd;
    int server_id;  // Which server owns this?
};

class Server {
    std::vector<Listener> listener;
};
```

**Problems:**
1. `server_id` is redundant - if Listener is owned by Server, Server already knows its own id
2. Listener is just wrapper around `int fd` with no additional behaviour
3. No methods, no logic - pure data holder

**Question:** Why not `std::vector<int> listen_fds` directly in Server?


## Decision: Remove Listener class?

**Proposal:** Yes. Replace with `std::vector<int> listen_fds` in Server.

**Rationale:**
- Listener adds no behaviour
- `server_id` field is redundant
- Direct vector is simpler and clearer

**Implementation:**
```cpp
// Before
class Server {
    std::vector<Listener> listener;
};

// After
class Server {
    std::vector<int> listen_fds;
};
```


-------------------------------------------------------------------------------

Lukas' comments:

Zu der Listener Class
Ich bin mir nicht sicher, wie viel Code Spezifisch zu listener da sein wird, 
das gehe ich jetzt an. wenn es wirklich kaum code ist, koennen wir es in Server
mergen... aber ich denke, besonders fuer eine gute Projekt Struktur ist es 
essentiell dass wir erst mal das ganze Getrennt lassen. 
Dadurch ist unser Server part schoen aufgeteilt. Server Sachen im server, 
Listener Sachen im Listener... und wenn wir merken dass es kaum etwas ist, 
koennen wir es immer noch mergen. aber so, jetzt glaube ich, 
dass es am besten ist wenn wir es erst mal trennen. 
Einfach fuer die Struktur und uebersicht.

Kleines Update: Ich habe einen Listener Namespace gemacht. 
Ich glaube, das ist der beste Trade Off. In dem koennen wir alles machen, 
was mit listener zu tun hat, ziemlich genau so wie du es wolltest, 
aber der Code ist dennoch seperat


-------------------------------------------------------------------------------

What is a Listener?
What is the thing that listens?

A listening socket is a passive endpoint — an fd that the OS has bound 
to an address and placed in the LISTEN state. 
Its sole behaviour is: when a connection arrives, 
produce a new fd via accept().

So the question becomes: does this behaviour warrant its own type?

The current Listener class
```cpp
class Listener {
    int fd;
    int server_id;
};
```

server_id is already logically redundant — if Listener lives inside Server,
the ownership is already encoded structurally. This field is phantom data: 
it exists because the relationship wasn't captured in the type hierarchy, 
so it was encoded as data instead.
And there are no methods. It's a struct holding an int.
This is not a class. It's a named int with a redundant annotation.


Lukas' proposal: Listener namespace
This is the interesting move. He's not arguing for a class with behaviour — 
he's arguing for a namespace as an organizational unit. 
Separate code, but not a separate type.

What does this mean concretely? A namespace groups free functions and data 
that conceptually belong together. So Listener::create(), 
Listener::register_with_epoll(), etc. — 
functions operating on int fd — would live here.

The honest question is: does "listener" carve out a real ontological boundary, 
or is it just a naming convention?

What a listener actually does in the lifecycle:

    1. Created from config (host:port → fd)
    2. Registered with event multiplexer (watching for POLLIN)
    3. On event: accept() → produces client fd → hands off to Connection creation
    4. Never closed until server shutdown

Steps 1 and 4 are Server initialization/teardown. 
Steps 2–3 are event loop logic. 
The "listener" behaviour is spread across phases that already belong to other entities.


The real tension
Lukas says: for good project structure, separate first, 
merge later if it turns out to be little.
This is a reasonable engineering heuristic but it contains a hidden assumption — 
that the cost of premature separation is lower than premature merging. 

That's not always true. Premature separation creates:

    Interface boundaries that don't reflect actual logical boundaries
    Questions about ownership that otherwise wouldn't exist 
    ("who creates the Listener? who destroys it?")
    Indirection that has to be justified

The counter-principle: a boundary should be drawn where there is a genuine interface 
— a point where one entity hands something to another and stops caring about internals. 
Does Server genuinely not care about how listener fds are created? 
Or does it need to know (for config-driven address binding, 
SO_REUSEADDR settings, non-blocking setup)?

If Server needs to configure every aspect of socket creation — 
and it does, because the address, port, options all come from Server's parsed config — 
then the listener fd creation is not encapsulatable away from Server.

Verdict
Lukas' instinct to separate concerns is correct in spirit. 
The namespace compromise is clever — 
it avoids the class instantiation problem while still organizing code.

But I'd push him on what specifically belongs in Listener namespace 
that couldn't belong in Server. If the answer is "just the socket setup syscalls," 
that's not a conceptual boundary — that's a file organization preference. 
You could achieve the same with Server_socket.cpp as an implementation file.

The Listener namespace is justified if there's meaningful logic 
that neither Server nor the event loop owns naturally.
If it ends up being Listener::create(host, port) → int, 
it's a thin wrapper and the namespace exists only for aesthetics. 
Which is fine for a 42 project — but should be named honestly as such.