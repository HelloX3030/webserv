## representation: no intermediate AST

### the question

should the parser build a general abstract syntax tree from which
`HttpRequest` is derived, or build `HttpRequest` directly?

### the analysis

an AST is valuable when:
- grammar has recursive structure that must be preserved for analysis
- later passes traverse/transform the tree (optimisation, type checking)
- multiple output representations derive from one parse

HTTP request processing:
- no recursive structure to preserve
- no transformation passes — request is consumed once
- single output: dispatch to handler

the target struct:
```cpp
struct HttpRequest {
    std::string method;
    std::string uri;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
};
```

this *is* the abstract representation. a tree with nodes for
RequestLine, Headers, Body would add indirection without information.
collapsing it yields exactly `HttpRequest`.

### the decision

build `HttpRequest` directly during parsing. no intermediate tree.

### the principle

representations exist to enable operations.
no operations require the tree → no tree.
