# c standard headers in c++


## the c-prefix convention

```cpp
#include <stddef.h>   // C: names in global namespace
#include <cstddef>    // C++: names in std::
```

`<cXXX>` wraps `<XXX.h>`.

prefer `<cXXX>` forms. use `std::size_t`, not bare `size_t`.


---


## `<cstddef>`

`std::size_t` — unsigned integer for sizes, counts, indices.
return type of `sizeof`. used throughout the standard library.

`std::ptrdiff_t` — signed integer for pointer differences.

`std::byte` — (C++17) raw memory access.


---


## `<cstdint>`

fixed-width integers: `std::int32_t`, `std::uint8_t`, etc.

use when bit-width matters: binary protocols, serialisation, hardware.
use `int` for general arithmetic.


---


## transitive inclusion is fragile

`<string>` may include `<cstddef>` in one implementation, not another.

include every header for every symbol used directly.
the include list is a dependency declaration.

see `header-files.md`.


---


## references

`<cstddef>`: https://en.cppreference.com/w/cpp/header/cstddef
`<cstdint>`: https://en.cppreference.com/w/cpp/header/cstdint
