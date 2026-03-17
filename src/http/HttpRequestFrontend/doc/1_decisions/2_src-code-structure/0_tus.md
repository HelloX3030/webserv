## file organisation: separate compilation units

### the question

should implementation be one file or multiple?
if multiple, what granularity?

### the analysis

ConfigFrontend uses fragment architecture: `.inc` files included into
a single `.cpp`. this keeps 1 translation unit while splitting source.
benefits: no link-time concerns, guaranteed inlining.
costs: unusual pattern, all-or-nothing recompilation...
  MORE RESEARCH NEEDED, E.G. INTO POTENTIAL PERFORMANCE CONCERNS

HttpRequestFrontend is smaller, the fragment architecture's complexity is not justified.

but separation by concern remains valuable:
- buffer management
- request-line parsing
- header parsing
- body consumption
- validation (if any)

these are distinct phases. separating them aids comprehension and
allows incremental development.

### the decision

separate `.cpp` files per phase, linked together (see fs).
internal header (`HttpRequestFrontend_internal.hpp`) defines `PhaseResult`
and any shared helpers.

### the principle

1 concern per file. link what belongs together.
complexity of organisation should match complexity of content.
