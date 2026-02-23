ctx: config parser: 1_read.cpp

# `std::istreambuf_iterator<char>` — slurping a file

```cpp
std::string source(std::istreambuf_iterator<char>(file),
                   std::istreambuf_iterator<char>{});
```

slurping: taking the entire contents of a stream into memory in 1
motion, as opposed to line-by-line or chunk-by-chunk.
onomatopoeic. part of the oral tradition — appears in documentation,
Stack Overflow, textbooks.

---

## anatomy

### the range constructor

`std::string` has a constructor:

```cpp
template< class InputIt >
basic_string( InputIt first, InputIt last );
```

accepts any 2 iterators satisfying InputIterator, reads `[first, last)`
into the string. `std::istreambuf_iterator<char>` satisfies this contract.

### `std::istreambuf_iterator<char>(file)` — begin

wraps the `std::streambuf*` underlying `file` (via `file.rdbuf()`).
positioned at the first unread character.
dereference → read 1 character. increment → advance.

### `std::istreambuf_iterator<char>{}` — end

default-constructed = end-of-stream sentinel.
signals "no more characters." the range constructor halts when
`first == last` — i.e. when begin reaches end-of-stream and becomes
equal to the sentinel.
does not point to a character. it is a signal.

---

## why not `std::istream_iterator<char>`

```cpp
// wrong for this use case:
std::string source(std::istream_iterator<char>(file),
                   std::istream_iterator<char>{});
```

`istream_iterator<char>` applies locale facets and skips whitespace
by default — spaces, tabs, newlines consumed silently.
fatal for a config parser: line-counting depends on `\n` surviving
the read intact.

`istreambuf_iterator` reads the raw `streambuf` directly, bypassing
the formatting layer. no locale. no skipping. every byte arrives.

the distinction:
`istream`      = formatted layer
`streambuf`    = raw byte layer beneath it

---

## further reading

cppreference.com:
. `std::istreambuf_iterator` → constructor, operator*, operator++
. `std::basic_string` constructors → InputIterator range form (overload 4)
. `std::istream_iterator` → contrast

---

## trade-off and culture

slurping = entire file in RAM. for config files: kilobytes, non-issue.
for multi-gigabyte logs: wrong tool.

imagery:
a string constructor, 2 iterators, 0 loops, 0 manual buffer management.
the iterator abstraction doing exactly what it was designed to do.