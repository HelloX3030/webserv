Dijkstra (change-rate)
Knuth (narrative)

separation of concerns
naming criteria
dependency direction

base/ criteria



## notes with concrete examples: v0 -> v1

http/ — request parsing, response building, method handlers
Tension here. 2 different concerns:
Protocol representation: Request parsing (bytes → HttpRequest), response building (HttpResponse → bytes). This is about HTTP's syntax and semantics.
Method handlers: What to do when receiving GET/POST/DELETE. This is behaviour.

Dijkstra's test: Do these change for the same reasons?
Protocol changes if RFC 1945/2616 interpretation changes
Handler behaviour changes if you change how DELETE works on your server
These are independent. Knuth would note the narrative confusion: "How does HTTP work?" shouldn't include "...and here's how we delete files."
