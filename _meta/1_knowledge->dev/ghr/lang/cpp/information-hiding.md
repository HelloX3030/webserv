Qs
. did Parnas really found modular programming or just rehash/popularise?
. "cost: heap allocation per instance, pointer indirection on access."
how costly? significance for programs?


# information hiding

## the principle

modules should hide design decisions.

a module exposes an interface — the minimum necessary for clients to use
it. everything else is implementation detail: hidden, changeable, not
depended upon.

origin: David Parnas, "On the Criteria To Be Used in Decomposing Systems
into Modules" (1972). the paper that founded modular programming.


## why it matters

3 concrete consequences of exposing implementation:

coupling: clients depend on internals. changing internals breaks clients.
the more exposed, the more breakage per change.

recompilation: in C++, any change to a header triggers recompilation of
all translation units that include it. exposed internals = more header
changes = slower builds.

cognitive load: readers must distinguish "what i can use" from "what
exists." exposed internals obscure the actual interface.

the inverse: hidden implementation can change freely. no client breakage,
no recompilation, no cognitive noise.


## C++ mechanisms

### access specifiers

```cpp
class Parser {
public:
    std::vector<ServerConfig> parse(const std::string& path);

private:
    std::vector<Token> tokens_;
    size_t pos_;
    Token peek() const;
    Token consume();
};
```

`private` hides members from clients. they cannot name or use them.

limitation: private members still appear in the header. clients cannot
use them, but clients see them. changes trigger recompilation.


### anonymous namespace

```cpp
// parser.cpp
namespace {
    enum class TokenType { STRING, LBRACE, RBRACE, END };
    struct Token { TokenType type; std::string value; };
    
    Token peek(const std::vector<Token>& tokens, size_t pos) { ... }
}
```

internal linkage. names invisible outside this translation unit.
not in any header. clients cannot see, cannot depend, no recompilation.

tradeoff: if state lives here, it has static duration. see
storage-duration.md for implications.


### PIMPL (pointer to implementation)

```cpp
// parser.hpp
class Parser {
public:
    Parser();
    ~Parser();
    std::vector<ServerConfig> parse(const std::string& path);

private:
    struct Impl;              // forward declaration only
    std::unique_ptr<Impl> impl_;
};

// parser.cpp
struct Parser::Impl {
    std::vector<Token> tokens_;
    size_t pos_;
    Token peek() const;
    Token consume();
    // all machinery here
};

Parser::Parser() : impl_(std::make_unique<Impl>()) {}
Parser::~Parser() = default;  // must be in .cpp where Impl is complete
```

header shows only the opaque pointer. all internals in .cpp.
clients see nothing. ABI stable. compile firewall complete.

cost: heap allocation per instance, pointer indirection on access.


---


## the spectrum

```
most exposed                                    most hidden
─────────────────────────────────────────────────────────────
public members    private members    anonymous ns    PIMPL
in header         in header          in .cpp         in .cpp
```

each step rightward: less coupling, less recompilation, more ceremony.

choose based on:
. how stable is the interface? (unstable → hide more)
. how many clients? (many → hide more)
. performance constraints? (PIMPL has indirection cost)
. is this a library or application code? (library → hide more)


---


## interface vs implementation

the boundary is not syntactic but semantic.

interface: what the module promises. stable contract. clients depend on
this.

implementation: how the promise is fulfilled. can change. clients must
not depend on this.

a private member in a header is syntactically hidden (cannot use) but
not physically hidden (can see, triggers recompilation). PIMPL achieves
both.


---


## other languages

Haskell: module system with explicit export lists. unexported names
are invisible to importers. no header/source split — the module itself
controls visibility.

```haskell
module Parser (parse) where  -- only parse is exported

data Token = ...             -- hidden
tokenise :: String -> [Token]  -- hidden
parse :: String -> Config    -- visible
```

Rust: privacy is per-item, default private. `pub` opts into visibility.
module boundaries enforce hiding. no header files — the source is the
interface, compiler extracts what's public.

```rust
mod parser {
    struct Token { ... }           // private to module
    fn tokenise(s: &str) -> Vec<Token> { ... }  // private
    
    pub fn parse(path: &str) -> Config { ... }  // public
}
```

Agda: similar to Haskell. modules with explicit exports. since Agda is
dependently typed and total, "hiding" also protects invariants — clients
cannot construct invalid states if constructors are hidden.


---


## the deeper point

information hiding is not about secrecy. it is about freedom.

hidden implementation = freedom to change without coordination.
exposed implementation = every change requires client agreement.

as systems grow, coordination cost dominates. hiding reduces the
coordination surface. this is why Parnas' principle scales.


---


## references

Parnas, D.L. (1972). "On the Criteria To Be Used in Decomposing Systems
into Modules." Communications of the ACM.

C++ Core Guidelines: C.9 (minimise exposure of members), I.27 (use PIMPL
for stable ABI)

Lakos, J. (1996). "Large-Scale C++ Software Design." — extensive
treatment of physical design and compile-time dependencies.