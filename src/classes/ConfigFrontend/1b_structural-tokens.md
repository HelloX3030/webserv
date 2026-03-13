## ctx

The tokeniser reduces the config file to a flat sequence of tokens.
The parser's job is to lift that flat sequence into a tree —
the parse tree whose leaves become ServerConfig / Location values.


## structural vs value tokens

To make that lifting possible, the grammar needs 2 kinds of tokens:

value tokens (STRING):
  carry semantic payload. They become actual values in the data structures.

structural tokens (SEMICOLON, LBRACE, RBRACE, END):
  carry no semantic payload. They are consumed and discarded;
  nothing in ServerConfig or Location ever holds one.


Their role is purely positional — they tell the parser where syntactic units begin and end.

## SEMICOLON

SEMICOLON specifically: it terminates a directive.
Without it, the parser cannot know where one directive's values end
and the next directive begins — because directives are variadic
(index takes 1+ filenames, allowed_methods takes 1+ methods).
The parser needs an unambiguous signal: "values are done here." SEMICOLON is that signal.

LBRACE / RBRACE delimit blocks — they encode depth change in the tree
(entering / leaving a server or location block).
SEMICOLON encodes breadth separation within a block — the boundary
between 1 directive and the next at the same depth.
END is the unique terminator of the entire input.

Formally: in a context-free grammar, SEMICOLON is a terminal symbol
whose only role is participation in production rules as a delimiter.
It carries no value into the semantic domain.
The grammar could in principle be redesigned to not need it —
fixed-arity directives, or indentation-sensitivity (Python, YAML) —
but then the burden shifts elsewhere.
SEMICOLON is the price of having variadic, unordered directives
in a flat-text config format.



## END sentinel and the enforcement gap

END is appended unconditionally by the tokeniser after the character
loop. postcondition: tokens_.back() is always END.

this makes every loop safe at stream exhaustion:
loops on `peek().type == STRING` exit when they reach END.
expect calls throw on END rather than consuming past the array.

the invariant is documented in the tokeniser's comment and relied
upon by every subsequent fragment. it is not enforced by the type
system. a future modification to tokenise() that omits the final
push_back — an early return, a refactored flush path — silently
breaks the contract. the downstream fragments cannot detect this
at compile time. failure manifests as a segfault on specific input.

minimum response: assert immediately after tokenise() in parse():
```cpp
assert(!f.tokens_.empty() &&
       f.tokens_.back().type == TokenType::END);
```

correct response: move the invariant into the type.
a TokenStream whose constructor guarantees END-termination
makes the comment redundant and the violation impossible.
