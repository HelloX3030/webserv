# ConfigParser.hpp — design notes

---

## file role

declaration of ConfigParser. answers: what is this class and
what is its public contract?

1 public method. all pipeline machinery is private.
Token and TokenType are private nested types — not part of the
public vocabulary. any file that includes this header to call
parse() compiles without knowing Token exists.

---

## includes

Config.hpp: required because parse() returns std::vector<ServerConfig>.
<string>, <vector>: used directly in the public signature.
nothing else is logically required at the interface level.

---

## Token and TokenType — private nested types

Token and TokenType are visible only inside ConfigParser.
3 options were considered:

1. free types in ConfigParser.hpp — exposes implementation details
   to every file that includes the header. triggers recompilation
   of all includers when Token changes.

2. separate internal header — correct scoping but adds a file whose
   only purpose is to hide what the class could own itself.

3. private nested types — class owns its pipeline machinery.
   Token and TokenType are access-controlled. implementation files
   reach them as ConfigParser::Token, ConfigParser::TokenType.

option 3 chosen.

TokenType::END is a sentinel appended by the tokeniser as its final
element. allows peek() to return a valid token at stream exhaustion
without an unchecked index. tokeniser postcondition: tokens_.back()
is always Token{END, "", last_line}.

---

## pipeline state: tokens_ and pos_

tokens_ and pos_ are the only shared mutable state.

they are object members, not locals passed through the call chain,
to eliminate tramp data: parameters carried through every level of
a 6-deep call chain not because those functions use them but because
something below does. tramp data obscures what each function actually
depends on and forces every call site to relay parameters. object
members eliminate this — every private method reaches shared state
through this implicitly.

---

## const on navigation helpers

peek() and at_STRING() are const: pure observations, no state change.
consume() and the expect_* family are not const: they advance pos_.

this makes the read/advance asymmetry visible in the declaration
without reading any implementation.

---

## expect_STRING() and expect_SEMICOLON()

these are not aliases for expect(TokenType::STRING/SEMICOLON).
they carry semantic context from the grammar position.

expect_STRING() is called where the grammar demands a directive value
or identifier. its error message says "expected directive value", not
"expected STRING". expect_SEMICOLON() always produces "expected ';'".

difference: reporting what the token stream received vs what the
grammar position requires. operator-facing, not type-system-facing.

---

## directive consumption contract

parse_server_dir and parse_location_dir consume the directive name
token before calling the specific parser.

rationale: the name token is spent as the dispatch decision. the
specific parser enters with pos_ at the first value token. it
handles values + semicolon and returns. this contract is consistent
across every specific parser — none consume their own name.

violating this contract produces off-by-one token errors. the
precondition is documented at parse_server_dir/parse_location_dir.

---

## parse_body_size_dir — single leaf, 2 call sites

parse_size(Token) → size_t is the single interpretation leaf.

2 call sites assign differently:
. server level: assigns directly to ServerConfig::client_max_body_size (size_t)
. location level: wraps in std::optional for Location::client_max_body_size

a shared intermediate method with size_t& cannot serve both.
the leaf is general; assignment is local to each caller.
the single name parse_body_size_dir is used at both call sites —
context of the enclosing directive parser (server vs location)
makes the differing assignment self-evident.

---

## [[nodiscard]]

applied to parse() and all private methods returning constructed types
(parse_config, parse_server_block, parse_location_block, parse_size,
parse_port, parse_host_port).

argument: these functions are pure transformations — their entire
purpose is the value returned. a caller that discards the return has
accomplished nothing observable. that is a logic error detectable
at compile time.

not applied to void methods or methods whose primary purpose is
side-effect (parse_server_dir, parse_listen_dir, etc.).

---

## deleted copy, default constructor

copy constructor and assignment: deleted.
ConfigParser holds mutable cursor state. copying a mid-parse parser
is semantically incoherent. deletion converts that bug into a
compile error.

move: compiler-generated. correct if ever needed.
default constructor: = default. no state to initialise before parse().

---

## return by value and NRVO

parse() returns std::vector<ServerConfig> by value.
caller owns the result — no ownership ambiguity, no pointer lifetime
questions.

C++17 NRVO (named return value optimisation) mandates that the
vector is constructed directly in the caller's destination. zero
copy cost. return by value is both ownership-correct and
performance-correct.