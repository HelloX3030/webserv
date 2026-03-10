# bear


## essence

bear (build event auditing relay) is a process auditor.
it intercepts compiler invocations during a build and emits
a record of each one as a JSON entry in `compile_commands.json`.

it is build-system agnostic: it does not parse Makefiles, CMake
scripts, or any build description. it operates below the build
system, at the OS dynamic linker level.


---


## mechanism

on Linux: `LD_PRELOAD` injection.
before any child process executes, the dynamic linker loads
bear's shared library into the process image. that library
interposes on the `exec` family of syscalls (`execve`, `execvp`,
etc.). when a `clang++` or `g++` invocation is detected,
bear records:
- the working directory at time of invocation
- the full argument vector (all flags, include paths, input file)
- the output file

on macOS: `DYLD_INSERT_LIBRARIES` (the ELF-equivalent mechanism
in the Mach-O/dyld runtime).

`LD_PRELOAD` and `DYLD_INSERT_LIBRARIES` are not POSIX.
they are de facto Unix conventions tied to the ELF and Mach-O
dynamic linkers respectively.


---


## output

`compile_commands.json` — the JSON Compilation Database format,
specified by the LLVM/Clang project.

```json
[
  {
    "directory": "/home/user/project",
    "command":   "clang++ -std=c++17 -Wall -Iinclude -c src/main.cpp -o obj/main.o",
    "file":      "src/main.cpp"
  }
]
```

this file is the standard interface between build systems and
analysis tools (clangd, clang-tidy, clang-check). any tool
that needs to understand how a TU is compiled reads this file.


---


## usage

run:
```bash
bear -- make          # generate compile_commands.json
bear -- make debug    # if build variant affects flags
```

when (etiquette)

project first cloned or set up

regenerate when compilation graph changes:
source files are added/removed, include paths
change, compiler flags change.
Makefile structure changes

not on every build (wasteful).


---


## transfer: the LD_PRELOAD interception pattern

the mechanism bear uses is general and significant beyond build tooling.

**dynamic linker interception** (LD_PRELOAD / DYLD_INSERT_LIBRARIES):
a shared library is injected into a process before execution.
functions in that library shadow symbols in libc or other libraries.
any call the target process makes to the interposed function
is routed through the injector first.

applications in security and systems:

- **malware analysis**: intercept `connect`, `send`, `open`,
  `execve` to observe what a binary does without modifying it.
  tools: `ltrace` (library call tracer), `strace` (syscall tracer),
  custom LD_PRELOAD shims.

- **API tracing / reverse engineering**: record all TLS, crypto,
  or network calls a closed binary makes — useful when you cannot
  inspect source.

- **sandboxing**: intercept and deny specific calls
  (e.g. block `open` on paths outside a root).

- **fuzzing harnesses**: intercept memory allocators to inject
  fault conditions; intercept `rand` to control non-determinism.

- **testing**: replace filesystem or network calls with stubs
  without modifying the binary under test.

the pattern's constraint: it operates at the dynamic linker layer.
statically linked binaries, or calls made directly via syscall
instructions (bypassing libc), are invisible to it. this is
a known limitation exploited in evasion techniques.


---


## references

bear source: https://github.com/rizsotto/Bear

JSON Compilation Database specification:
https://clang.llvm.org/docs/JSONCompilationDatabase.html

LD_PRELOAD mechanism: `man ld.so` (Linux), `man dyld` (macOS)



## further info to explore, esp. security-relevant

LD_PRELOAD operates at the dynamic linker layer, not the syscall layer. It interposes on libc wrapper functions — execve(3) the C function — not execve(2) the raw syscall.

The critical implication: a process can bypass LD_PRELOAD entirely by invoking syscalls directly via syscall instruction (x86-64: syscall opcode; x86: int 0x80) rather than going through libc. This is a known evasion technique — malware that avoids detection by LD_PRELOAD-based monitors does exactly this.

So: bear requires the dynamic linker and libc to be in the call path. Statically linked binaries, or binaries that make direct syscalls, are invisible to it. For build auditing this doesn't matter — compilers always use libc. For security monitoring it matters enormously.

True syscall-level interception requires ptrace — which is what strace uses. ptrace intercepts at the kernel boundary, below any userspace evasion. Bear 3.x actually explored this direction for robustness, though it adds complexity (ptrace requires elevated privileges or CAP_SYS_PTRACE in the general case).
