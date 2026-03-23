## predicate

configuration frontend.
read → tokenise → parse → validate → serialise.

transforms config file text into validated `ServerConfig` structures.

---

## contents

```
Config.cpp              schema, data structures (ServerConfig, Location)
ConfigFrontend/         frontend pipeline
```

batch processing. pure function: input text, output config or error.

---

## naming

"config" — the concern.
