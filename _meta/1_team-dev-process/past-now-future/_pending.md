## purpose

Asynchronous communication of structured feedback on specific code locations.


## Qs: G > L

### .gitignore

possibly irrelevant?

  Fortran (.mod, .smod)

  Windows (.exe, .dll, .pdb, .ilk)

  Library files (.so, .dylib, .a, .lib):
    we're building an executable, not distributing libraries.
    If we link against external .so files, they shouldn't be in our project directory anyway?

  .out pattern:
    overly broad.
    executable is specifically named webserv

  *.pdf
    does Lukas keep 42's subject.pdf locally?


Principle:
Ignore ONLY:

  Build outputs - anything reproducible from source
  Personal environment - editor/OS files that vary per developer


Possible better minimal approach:

# Build artifacts
*.o
*.d
obj/
webserv