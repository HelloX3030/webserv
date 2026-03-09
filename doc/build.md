# build

webserv has 3 build variants. each produces a separate binary
and a separate object directory.

---

## variants

### release — `make` or `make all`

standard build. full warnings, errors-as-warnings, C++17.
no debug instrumentation.
normal server usage

produces: `webserv`

### debug — `make debug`

adds `-DDEBUG=1`. enables conditional debug output in the
source. use for:
- tracing server behaviour during development
- understanding control flow without a debugger

produces: `webserv_debug`

### leaks — `make leaks`

adds `-DDEBUG=1 -g -O0 -fno-omit-frame-pointer`. disables
optimisation and preserves stack frames so valgrind can
produce accurate traces. use for:
- memory leak detection
- file descriptor leak detection

produces: `webserv_leaks`

---

## running

each variant has a paired run target:

```
make run          # builds release if needed, then runs ./webserv
make debugrun     # builds debug   if needed, then runs ./webserv_debug
make leaksrun     # builds leaks   if needed, then runs valgrind ./webserv_leaks
```

`leaksrun` runs:

```
valgrind --leak-check=full --track-fds=yes --show-leak-kinds=all ./webserv_leaks
```

---

## cleaning

```
make clean        # remove all object directories
make fclean       # remove object directories and all binaries
make re           # fclean, then rebuild release
```

per-variant if you only want to touch 1 variant:

```
make debugclean   make debugre
make leaksclean   make leaksre
```

---

## verbosity

by default the build is silent: 1 progress line per file.

```
make V=1          # full command echo for release build
make V=1 debug    # full command echo for debug build
```

use `V=1` when diagnosing a compiler error: the complete
invocation is printed and can be run manually.

---

## important: flag changes require `make re`

make uses file modification times to detect staleness.
it does not detect changes to compiler flags. if you
modify `CXXFLAGS` or any flag variable, existing object
files will appear up-to-date and will not be recompiled.

after any flag change: run `make re` (or the variant-specific
`re` target) before the next build. this is a discipline
requirement.