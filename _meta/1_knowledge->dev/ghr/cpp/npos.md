# std::string::npos — the "not found" sentinel

---

## what it is

`npos` is a static constant member of `std::string` (and
`std::basic_string`). it represents "no position" — the sentinel
value returned when a search fails.

```cpp
static const size_t npos = -1;
```

`-1` assigned to unsigned `size_t` wraps to the maximum value
(`SIZE_MAX`, typically `18446744073709551615` on 64-bit systems).

---

## why it exists

`find()` and related methods return an index on success. on failure,
they need a value that:

1. cannot be a valid index (no string can have `SIZE_MAX` characters)
2. is the same type as the success return (`size_t`)
3. is a named constant (not a magic number)

`npos` satisfies all 3.

---

## the problem it solves

### why not return -1 directly?

`find()` returns `size_t` (unsigned). comparing unsigned to signed
`-1` is problematic:

```cpp
if (s.find(':') == -1)  // warning: comparison of unsigned with signed
```

the comparison works due to implicit conversion, but compilers warn
and the intent is unclear.

### why not return a bool?

sometimes you need both: did it exist, and where? `find()` answers
both in 1 return value. `npos` means "no", any other value means
"yes, at position N".

### why not use std::optional?

`std::optional<size_t>` would be semantically clean. but `find()`
predates `std::optional` (C++17) by decades. also, returning a raw
`size_t` is marginally faster (no wrapper overhead, though likely
optimized away).

---

## usage pattern

```cpp
std::string s = "host:port";

if (auto pos = s.find(':'); pos != std::string::npos)
{
    // colon found at index `pos`
    std::string host = s.substr(0, pos);
    std::string port = s.substr(pos + 1);
}
else
{
    // no colon
}
```

the idiom: call `find()`, compare result to `npos`.

---

## methods that return npos on failure

```cpp
size_t find(...)           // first occurrence
size_t rfind(...)          // last occurrence
size_t find_first_of(...)  // first char in set
size_t find_last_of(...)   // last char in set
size_t find_first_not_of(...)
size_t find_last_not_of(...)
```

all return `npos` when no match exists.

---

## npos as "until end"

`npos` also means "until the end" in some contexts:

```cpp
s.substr(5, std::string::npos);  // from index 5 to end
s.substr(5);  // same (npos is the default)
```

---

## type

`npos` is `std::string::size_type`, which is typically `size_t`.
on most systems: unsigned 64-bit integer.

---

## common errors

### comparing to -1 or 0

```cpp
if (s.find(':') == -1)   // works but warns, unclear
if (s.find(':') == 0)    // wrong: 0 is a valid position (first char)
if (!s.find(':'))        // wrong: 0 converts to false, but means "found at position 0"
```

    "Falsy/truthy" — JavaScript jargon for implicit boolean coercion. 
    more explicitly: in C++, integer 0 implicitly converts to false in boolean context. 
    The value 0 IS a valid position. The boolean interpretation and the semantic meaning diverge.

### forgetting the check

```cpp
size_t pos = s.find(':');
s.substr(0, pos);  // if not found, pos is npos → huge substring
```

always check against `npos` before using the index.

---

## summary

`npos` is the type-correct sentinel for "not found" in string search
operations. it is `size_t(-1)`, which wraps to the maximum unsigned
value. compare search results to `std::string::npos` explicitly.