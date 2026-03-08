how shared state is passed through a call tree.

tramp-data as a named section or sub-node. 

e.g. ConfigFrontend.cpp:
6-level recursive descent call tree with 2 cursors
(tokens_ and pos_): passing both explicitly through every level
makes them tramp data — passengers in every signature not because
the immediate callee needs them but because something below does.
struct membership eliminates the tramp: methods reach shared state
via `this` implicitly.


Related upstream concepts: explicit vs implicit state, closure capture, the Reader monad in Haskell (where "threading" is made structurally explicit and first-class). The problem tramp data names: mismatch between where state is introduced, where it is used, and how many call frames it must traverse between the two. The cost is false coupling — every intermediate signature appears to depend on the data, signalling a dependency that does not exist.