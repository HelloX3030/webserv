## v0

initial setup from Lukas
to get the development underway


base/ and classes/ (then expanded also to include interfaces/).
more "syntax" oriented

once classes/ started bloating, became clear that new organisation was necessary


## v1

reorganisation






we're going to consider & redo from first principles the structure & naming of the source code. previously Lukas had created 2 temporary dumps: base/ and classes/, then also interfaces/. the last 2 were obviously too superficially focused on syntax and this was in dire need of improvement.
now i want a domain/concern/semantic-logical organisation, really considering the ontological essence & telos of each module. so that the new subdirectories within src/ (and later, obviously, matched in inc/) represent clear logical module boundaries.
