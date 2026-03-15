## location within file system

the directory: `1_webserv_v0-v1`
belongs in ./meta/1_knowledge-dev/architecture/src_organisation/
alongside 0_general/
and will be transferred there asap.

it is temporarily placed here so Lukas has direct access
and can more easily overview all decisions made re. reorganisation of source
alongside the actual source (by which I mean "code"/text)
from our initial v0 to this v1

---

## v0

### wtf was this

initial setup from Lukas
to get the development underway

### categories:

base/
  for foundational infrastructure, "utilities"

classes/

expanded also to include interfaces/

  more "syntax" oriented

once classes/ started bloating, became clear that new organisation was necessary

---

## v1

reorganisation.

main organisation principle: semantic-logical cohension
organise, group the program's main functional entities by domain / concern.

see ./meta/1_knowledge-dev/architecture/src_organisation/0_general
for general/upstream principles. (WIP)

each new category in v1 (subdirectories within src/)
has a markdown file with some information, including
reasoning on its name, contents, what's included/excluded compared with v0...

### overview


