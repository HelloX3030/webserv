# free functions vs member functions


## the question

should phase-parsing functions (`parse_request_line`, `parse_header_line`,
`consume_body`) be member functions of `HttpRequestFrontend` or free
functions taking the frontend as parameter?


## the analysis + decision

### ctx

what are these functions?

```cpp
PhaseResult parse_request_line(/* ... */);
PhaseResult parse_header_line(/* ... */);
PhaseResult consume_body(/* ... */);
```

they:

  read from buffer_
  write to request_ fields
  modify phase_, body_remaining_, error_code_
  return a result indicating what happened

they are stateful transformations of HttpRequestFrontend.

---

### member function model
```cpp
struct HttpRequestFrontend {
private:
    PhaseResult parse_request_line();
    PhaseResult parse_header_line();
    PhaseResult consume_body();
};
```

implicit this access to all fields
natural C++ idiom for "operations on an object"
private visibility enforces internal-only use
definition can be in any TU that includes the header

---

### free function model

```cpp
// internal header
PhaseResult parse_request_line(HttpRequestFrontend& self);
```

explicit state parameter
closer to functional style: transformation is visible in signature
requires either friend or public fields
more amenable to testing in isolation (pass mock state)


### the ontological question

what is parse_request_line?

option A: a method of the object — "the frontend parses its request line"
option B: a function over the state — "this function transforms frontend state"

both are valid framings. the difference is emphasis:

A emphasises the object as agent
B emphasises the transformation as primary


### practical considerations

encapsulation: member functions access private fields naturally.
free functions require either:

  public fields (breaks encapsulation)
  friend declarations (couples internal header to struct)
  accessor methods (boilerplate)


testability: free functions can be tested by constructing state directly.
member functions require constructing the object.
for HttpRequestFrontend, the object is the state — no difference.


C++ idiom: member functions for operations that transform this.
free functions for operations that don't privilege one argument.


### ConfigFrontend precedent
uses a namespace with free functions,
but ConfigFrontend is a namespace exposing parse() — stateless.
the internal Frontend struct uses member functions.


### verdict

member functions. reasons:

these are operations of the frontend, not operations on it from outside
this->buffer_, this->phase_ is cleaner than self.buffer_, self.phase_
private visibility in the header; no internal header needed for signatures
matches the pattern: advance() is public member, these are private members
the struct already exists to hold state — methods are the natural interface to that state


---


## the principle

operations intrinsic to an object's purpose are methods.
operations that happen to use an object are free functions.
parsing is intrinsic to a parser.
