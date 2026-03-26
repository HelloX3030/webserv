# ParseResult encoding

## problem

`ParseResult` has conditional validity:
- `request` meaningful iff `status == Complete`
- `error_code` meaningful iff `status == Failed`

the fields exist in all cases. validity is documented, not enforced.

## the ideal: sum types with per-variant payloads

in type-theoretic terms:
```haskell
data ParseResult
  = Incomplete
  | Complete HttpRequest
  | Failed ErrorCode
```

each constructor carries exactly its relevant data.
accessing `HttpRequest` from a `Failed` value is a type error —
the field does not exist in that variant.

this is algebraic data types (ADTs): sum types where each
summand has its own product structure. not dependent types
(types indexed by values) — simpler.

Rust, Haskell, OCaml, Agda express this directly.

## why C++ cannot express it

C++ historically separated:
- `enum` — tags without payloads
- `union` — payloads without tags (unsafe, no discriminant)
- `struct` — products only

no first-class tagged unions.

`std::variant<Incomplete, Complete, Failed>` approximates ADTs,
but access is verbose (`std::visit`, `std::get_if`) and there
is no pattern matching syntax. the encoding is correct;
the ergonomics are poor.

## decision

product with discriminant. the standard C++ encoding:
```cpp
struct ParseResult
{
    ParseStatus status;
    HttpRequest request;    // valid iff Complete
    uint16_t    error_code; // valid iff Failed
};
```

invariants documented, not enforced.
caller must respect the discriminant.

this is the "poor man's sum type" — universal in C and C++ codebases.
acceptable given language constraints.

## principle

when a language lacks a construct, encode it explicitly and document
the invariant. do not pretend the limitation does not exist.
