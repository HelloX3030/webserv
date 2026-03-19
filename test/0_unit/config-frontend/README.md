# config-frontend test

test driver for ConfigFrontend. parses a config file, prints the resulting
ServerConfig structs via to_string().

validates the complete pipeline: read → tokenise → parse → validate → serialise.


## build

```bash
cd test/config-frontend

make          # release
make debug    # gdb-ready
make leaks    # valgrind-ready
```


## run

config file specified via CFG variable. default: `config/valid/default.conf`.

```bash
# release
make run
make run CFG=../../config/valid/cgi.conf
make run CFG=../../config/valid/multi-server.conf

# debug (gdb-ready)
make debugrun
make debugrun CFG=../../config/valid/uploads.conf

# valgrind
make leaksrun
make leaksrun CFG=../../config/valid/uploads.conf
```


## iterate across all valid configs

```bash
for f in ../../config/valid/default.conf \
         ../../config/valid/cgi.conf \
         ../../config/valid/uploads.conf \
         ../../config/valid/multi-server.conf; do
    echo "=== $f ==="
    make run CFG="$f"
done
```


## what this verifies

1. the pipeline works end-to-end: read → tokenise → parse → validate
2. every field in ServerConfig and Location is populated (or defaulted)
3. to_string covers all fields — any field missing from the output reveals an omission


## what it cannot verify

C++17 has no reflection. the compiler cannot detect a field added to a struct
but omitted from to_string. visual inspection against Config.hpp is the only guard.
