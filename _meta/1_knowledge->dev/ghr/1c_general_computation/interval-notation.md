# interval notation

[a, b) — mathematical convention for ranges:

[ = inclusive (closed)
( or ) = exclusive (open)

[0, 5]   →  0, 1, 2, 3, 4, 5      includes both endpoints
[0, 5)   →  0, 1, 2, 3, 4         includes 0, excludes 5
(0, 5]   →  1, 2, 3, 4, 5         excludes 0, includes 5
(0, 5)   →  1, 2, 3, 4            excludes both

C++ ranges are conventionally [begin, end) — 
start inclusive, end exclusive. substr(pos + 1) means [pos + 1, end).