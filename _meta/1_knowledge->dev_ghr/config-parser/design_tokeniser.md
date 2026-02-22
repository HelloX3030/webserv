# tokeniser design

---

## what the tokeniser must do

the tokeniser receives a string of characters.
it must produce a flat sequence of tokens.

a token is the smallest unit of meaning in the grammar:
a structural char:
`{`
`}`
`;`
or an arbitrary character sequence (a STRING).

the question is: how do you transform a linear stream of characters
into a sequence of categorised units?

---

## a naive approach — and why it fails

```cpp
std::vector<std::string> words = split(source, " \t\n");
for (auto& word : words)
    // scan word for { } ;
```

split the source by whitespace first. get "words".
then scan each word for structural characters.

this fails on any input where a structural character is not
surrounded by whitespace — where a STRING and a structural char
are adjacent with no delimiter between them.

consider `server{` or `root/var/www;`.
the splitter produces a single element in each case.
the scanner then has to find the boundary inside that string —
which means reimplementing char-by-char scanning inside a loop
that already committed to the wrong unit.

the patched version ends up doing:

```cpp
for (auto& word : words)
{
    std::string current;
    for (char c : word)
    {
        if (c == '{' || c == '}' || c == ';')
        {
            if (!current.empty()) emit STRING(current);
            emit structural(c);
            current.clear();
        }
        else current += c;
    }
    if (!current.empty()) emit STRING(current);
}
```

this is char-by-char scanning — the correct approach — executed
inside a word-splitting loop that adds nothing and breaks on
`server{`. the outer loop is vestigial, a wrong first move
that the correct inner loop renders irrelevant.

the lesson: the natural unit of analysis for this grammar is the
character. word-splitting presupposes a boundary structure (whitespace)
that this grammar does not have.

---

## the automaton — 2 states

the tokeniser is a finite automaton.

it has exactly 2 states:
- accumulating: currently inside a STRING token.
- idle: between tokens, no characters accumulated.

transitions:

```
idle + ordinary char     → accumulating (start new STRING)
accumulating + ord. char → accumulating (extend STRING)
accumulating + whitespace → idle (emit STRING, clear)
accumulating + struct.    → idle (emit STRING, emit structural)
idle + whitespace         → idle (skip)
idle + structural char    → idle (emit structural)
source exhausted          → (flush if accumulating, emit END)
```

the automaton has no memory beyond: what have I accumulated so far?
that is `current`. the state is implicitly encoded in `current.empty()`.

why only 2 states? because the grammar has only 2 token classes
with different boundary conditions:

structural tokens —
`{`
`}`
`;`
— are self-delimiting.
1 character uniquely determines the token. no accumulation needed.
emit immediately.

STRING tokens are variable-length with no intrinsic terminator.
the end of a STRING is determined by what comes after it —
not by any property of the STRING itself.
this requires accumulation: collect characters until a non-STRING
character ends the token.

---

## flushing — what it is and why it is necessary

the accumulator `current` collects characters for a STRING token
in progress. it is a buffer.

the tokeniser cannot know a STRING is complete until it sees a
character that is not part of a STRING: whitespace, a structural
char, or end-of-source. that character is a boundary event.

at a boundary event, 2 things must happen in order:
1. emit the accumulated STRING (if any) to the token list.
2. clear the accumulator.
3. handle the boundary character itself.

this sequence is a flush. "flush" means: drain the buffer to its
destination, then clear it.

order is not negotiable.
flush after structural: the STRING is lost.
structural before flush: the STRING is emitted after the structural
token, reversing the order of `server` and `{`.

the flush-before-emit invariant:
every boundary event first flushes `current`, then handles itself.

without this invariant, `server{` produces either:
- STRING("server{") — structural char swallowed into STRING.
- LBRACE then STRING("server") — tokens reversed.
- LBRACE — STRING silently dropped.

all 3 are bugs. the invariant prevents all 3.

the concept generalises: any time data is accumulated in a buffer
that must be drained before processing a boundary, the pattern is flush.

---

## the flush lambda — syntax and semantics

```cpp
auto flush = [&]()
{
    if (!current.empty())
    {
        tokens_.push_back({TokenType::STRING, current, line});
        current.clear();
    }
};
```

### what a lambda is

lambda calculus (λ-calculus) was invented by Alonzo Church in 1936
as a formal system for expressing all computation through function
abstraction alone. a function in λ-calculus is written:

```
λx. x + 1
```

meaning: a function that takes x and returns x + 1.
no name. the function is a value, not a reference to a named thing.

Church proved this was equivalent in expressive power to Turing
machines. every computation can be expressed as lambda abstraction.

when C++ use "lambda", they mean: a function defined as a value 
at the site of use, not named elsewhere. an anonymous function.

in C++, the compiler generates a unique unnamed class for each
lambda expression. the class has an `operator()` — the call
operator. what your editor shows as `(lambda -> void)` is the
compiler's internal representation of that generated type.
`auto` is required because the type has no writable name.

### the capture clause `[&]`

the `[&]` is not general C++ syntax — it is lambda-specific.
it is what syntactically identifies a lambda expression to the
compiler. without `[&]`, there is no lambda. it is the `λ` in
Church's notation.

it declares what variables from the enclosing scope the lambda
body is allowed to access, and how.

```
[]              — capture nothing.
[&]             — capture everything in scope by reference.
[=]             — capture everything by value (copies).
[&x, y]         — capture x by reference, y by value.
[this]          — capture the enclosing object's this pointer.
```

`[&]` in `flush` means: `current`, `line`, and `tokens_` inside
the lambda body are not copies — they are the actual variables in
`tokenise`. mutations inside `flush` mutate them in `tokenise`.

this is logically required: `flush` must clear `current` after
emitting. if `[=]` were used, `flush` would operate on a copy of
`current`. clearing the copy leaves the original untouched.
the accumulator never empties. every STRING token emits all
characters accumulated since program start. the bug is silent
until the token list is inspected.

the `()` is the parameter list. empty: `flush` takes no arguments.
it accesses everything it needs through captured references.

### why a lambda rather than a private method?

`flush` needs `current`, `line`, and `tokens_`.

as a private method, these would have to be parameters:

```cpp
void flush(std::string& current,
           std::vector<Token>& tokens_,
           size_t line);
```

this is tramp data — parameters carried through a call chain not
because the function has independent inputs, but because the caller
owns state the callee needs. it obscures what `flush` actually
depends on: the call site must relay 3 variables every time.

`tokens_` and `pos_` are already class members to eliminate this
pattern at the parse phase. `current` and `line` are locals in
`tokenise`. they cannot be class members without leaking transient
tokeniser state into the object's interface.

the lambda closes over them naturally. `flush` lives at exactly
the scope where it is needed and nowhere else. the call site is
`flush()` — 7 characters. the dependency is encoded in the capture
list, not in a parameter signature.

a closure is a function that carries its environment with it.
`flush` is a closure over `current`, `line`, and `tokens_`.

### why not a named helper function?

a free function or static private method cannot capture local
variables. it has no enclosing scope to capture from.
it would require the tramp-data parameter approach above, or
the variables would have to become class members — which is worse.

a named function also implies the operation exists independently
of its context, is reusable, has meaning in isolation. `flush`
is not reusable. it is a local operation defined by its context.
making it a lambda makes this explicit: the function exists only
here, for this purpose, with these captured variables.

contrast with manual C-style lexers: every operation that needs
shared state either takes it as explicit parameters (tramp data)
or accesses global/struct pointers threaded through by hand.
closures, RAII, and the C++17 initialiser-if are abstractions 
that compress the semantic distance between
intent and expression. the lambda here replaces ~10 lines of
manual state threading with a name and a call.

---

## the `\r` decision

`\r` (carriage return) is the first character in Windows line
endings (`\r\n`). the decision: don't handle it.
document a precondition in phase 0: unix line endings only.

the server runs on Linux. implementing `\r` handling means
claiming a guarantee for a case we do not test — dead code that
implies a false contract.

---

## the END sentinel

```cpp
tokens_.push_back({TokenType::END, "", line});
```

appended unconditionally as the final element of `tokens_`.

the parser navigates `tokens_` using `peek()` and `consume()`.
`peek()` returns the token at `pos_` without advancing.

without END, `peek()` at stream exhaustion requires:

```cpp
Token peek() const {
    if (pos_ >= tokens_.size())
        // what do we return here?
```

options:
- throw an exception — but stream exhaustion is not an error
  at the observation level. it is an error only in context.
- return a default-constructed Token — but TokenType has no
  "none" value without adding one.
- check at every call site — callers must test before calling.
  this scatters the boundary check across the parser.

END eliminates the problem. `peek()` at exhaustion returns the
END token. the parser loop terminates on END. no bounds check.
no special case. `peek()` is unconditionally safe.

postcondition of `tokenise()`:
`tokens_.back()` is always `Token{END, "", last_line}`.
this postcondition makes every downstream function's behaviour
predictable without checking `pos_` against `tokens_.size()`.

---

## `tokens_.clear()` at entry

`tokenise()` is called once per `parse()` invocation.
`tokens_` is always empty at entry.

the clear is defensive: `tokenise()` is correct independently
of its call site. if the contract (called once, after construction)
ever breaks, the clear prevents silent corruption.
cost: negligible. benefit: the method does not depend on caller
discipline to be correct.

---

## `current += c` versus `current.push_back(c)`

identical observable behaviour for `char`.

`current += c` reads as: extend this string with this character.
accumulation.

`current.push_back(c)` reads as: append to this container.
a container operation.

we are building a lexeme — a meaningful character sequence.
the mental model is accumulation, not container manipulation.
`+=` expresses the intent.

---

## summary: why this design

the char-by-char automaton is correct because:
- the grammar's boundary events occur at the character level.
- no higher-level split presupposes a boundary structure
  the grammar does not guarantee.

the flush invariant is necessary because:
- STRING tokens have no intrinsic terminator.
- their end is determined by what follows, not by their content.
- the boundary event must drain the accumulator before handling itself.

the lambda is correct because:
- `flush` is a closure over local state, not an independent operation.
- capturing by reference is logically required for mutation.
- the tramp-data alternative is worse at every call site.

the END sentinel is correct because:
- it makes `peek()` unconditionally safe.
- it moves boundary handling from scattered call sites to 1 location.
- it makes the tokeniser postcondition expressible and checkable.