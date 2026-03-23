## name

the term "organisation" subsumes:

structure (spatial arrangement),
naming (identification),
boundaries (what belongs where),
and dependencies (what knows about what).

It's the act of arranging parts into a functioning whole —
from Greek organon, an instrument or organ. The parts serve the whole.


could potentially also include? :
file naming conventions, header/source split reasoning, fragment files, namespace strategy.
or is this more "design / style"?


## threads to follow, things to research

narrative
(Knuth)

Structure reveals logic 

separation of concerns

Clean interfaces

naming criteria
precise...

dependency direction
No cyclic dependencies

change-rate
(Dijkstra)


general base/ criteria



## notes with concrete examples: v0 -> v1. from AI thread, to be processed asap


organise by domain, NOT by role, e.g. NOT putting the following together: frontend/Config, frontend/HttpRequest

Dijkstra's lens: separation of concerns
Dijkstra's criterion: separate things that change for different reasons.

When does ConfigFrontend change?
Config file format changes
Validation rules change
New directives added

When does HttpRequestFrontend change?
HTTP protocol requirements
Request handling edge cases
New methods supported

These change independently. They share parsing techniques but not parsing content. The concern being separated is domain, not mechanism.

Dijkstra would also note: both frontends are pure transformations.
load_config : Path → Config
parse_request : Bytes → HttpRequest

No side effects, no state. This purity is what makes them "frontends" — they transform representations. But purity is a property, not a location.


Knuth's lens: literate structure
Knuth asks: what makes the program readable as a narrative?

If someone asks "how does configuration work?", they should find:
The config grammar/schema
The parser
Validation logic
The resulting Config type
Access patterns
All in one place. The story of configuration is coherent.

Scattering ConfigFrontend into a frontend/ directory would break the narrative. Now you must cross-reference: "the parsing is over there, but the type is over here, and the validation is..."
Resolution
Domain wins. Role is encoded in the name.







http/ — request parsing, response building, method handlers
Tension here. 2 different concerns:
Protocol representation: Request parsing (bytes → HttpRequest), response building (HttpResponse → bytes). This is about HTTP's syntax and semantics.
Method handlers: What to do when receiving GET/POST/DELETE. This is behaviour.

Dijkstra's test: Do these change for the same reasons?
Protocol changes if RFC 1945/2616 interpretation changes
Handler behaviour changes if you change how DELETE works on your server
These are independent. Knuth would note the narrative confusion: "How does HTTP work?" shouldn't include "...and here's how we delete files."
