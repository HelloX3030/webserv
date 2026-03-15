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
