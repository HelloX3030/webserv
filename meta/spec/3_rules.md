# Chapter II: General rules
. Your program must not crash under any circumstances
(even if it runs out of memory) or terminate unexpectedly.
If this occurs, your project will be considered non-functional
and your grade will be 0.
. You must submit a Makefile that compiles your source files.
It must not perform unnecessary relinking.
. Your Makefile must at least contain the rules:
$(NAME), all, clean, fclean and re.
. Compile your code with c++ and the flags -Wall -Wextra -Werror
. Your code must comply with the C++ 98 standard and should still compile when
adding the flag -std=c++98.
  NB: 42 Heilbronn campus allows C++17
. Make sure to leverage as many C++ features as possible (e.g., choose <cstring>
over <string.h>). You are allowed to use C functions, but always prefer their C++
versions if possible.
. Any external library and Boost libraries are forbidden.