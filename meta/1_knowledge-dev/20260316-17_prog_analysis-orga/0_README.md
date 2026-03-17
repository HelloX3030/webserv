# UPDATE 20260317

Lukas wants to separate the routing logic, and own it.
this documentation needs updating.

see:
  GitHub Issue #5
  20260317-1
  src/http/HttpRequestFrontend/doc/
    esp. 3_integration.md



# program analysis

reverse-engineering Lukas's webserv implementation.
goal: understand it, then integrate cleanly.


---


## purpose

1. document what exists — types, data flow, ownership
2. identify architectural debt — coupling, duplication, conflation
3. specify v1 integration — minimal adaptation to existing infrastructure
4. sketch v2 design — first-principles rebuild


---


## structure

```
1_current-state/        what Lukas built
2_v1-integration/       what ghr must do now
3_v2-design/            what ghr will build later
```


---


## scope

runtime, handlers, response building.
excludes ConfigFrontend (ghr's, already documented).
excludes HttpRequestFrontend (ghr's, in progress).
