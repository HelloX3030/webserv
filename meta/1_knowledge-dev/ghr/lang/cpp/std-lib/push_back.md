https://en.cppreference.com/w/cpp/container/vector/push_back

Standard library — std::vector<T>::push_back(const T&).

Essence: append one element to the end of a dynamic array. The vector owns a heap-allocated buffer of capacity ≥ size. push_back writes the new element at buffer[size], increments size. When size would exceed capacity, the vector allocates a new buffer (typically 2× current capacity), moves all existing elements, then writes the new element. The old buffer is freed.

Telos: amortised O(1) append. The occasional reallocation is O(n), but averaged across n pushes the cost per element is O(1). This is the fundamental reason vectors are the default sequential container.


within ConfigFrontend : parser, memory-related:
push_back may invalidate all iterators and pointers into the vector if reallocation occurs. We hold no iterators into tokens_ or s.listen etc. during any parse call, so there is no hazard here.
