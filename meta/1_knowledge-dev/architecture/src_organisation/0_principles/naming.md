directory = namespace/domain (lowercase)
file = type (PascalCase)
precedent (Boost, LLVM, kernel)
the ontological distinction: containers vs instantiables




Directories are not types. They're filesystem namespaces — containers that group related entities.
PascalCase semantically implies "this is instantiable, this is a noun-as-type." A directory is neither.



Precedent from serious C++ projects:

project
directory style

  C++ standard library
  lowercase (bits/, ext/)

  Boost
  lowercase (boost/asio/, boost/spirit/)

  LLVM/Clang
  lowercase or mixed, but not class-style

  Linux kernel
  lowercase

  nginx
  lowercase









from ctx: Http Request Frontend:

filename prefix
with prefix: HttpRequestFrontend_1_buffer.cpp
without: 1_buffer.cpp
arguments for prefix:

unambiguous in build logs, error messages, grep results
Knuth: names should be self-documenting in isolation
if a file ends up in a flat listing (IDE tabs, find output), context preserved

arguments against:

redundant — directory already provides context
longer names, more typing
Unix tradition: let the path do the work

my read: the prefix adds value precisely because build systems, editors, and tools often strip path context. seeing HttpRequestFrontend_3_headers.cpp:42: error is clearer than 3_headers.cpp:42: error.
verdict: keep the prefix. it costs nothing at runtime and aids debugging.
