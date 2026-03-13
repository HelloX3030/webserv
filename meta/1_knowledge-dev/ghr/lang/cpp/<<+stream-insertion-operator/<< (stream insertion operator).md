# stream insertion operator <<

## definition

`<<` is the stream insertion operator for output.

```cpp
std::cout << "hello";
std::cout << 42;
std::cout << variable;
```

## what it does

sends data to an output stream.

```cpp
std::cout << x;
```

means: insert value of x into the standard output stream.

## mathematical view

function that takes 2 arguments:

```
operator<< :: OutputStream → Value → OutputStream
```

takes stream, takes value, returns stream (allowing chaining).

## chaining

because `<<` returns the stream, you can chain:

```cpp
std::cout << "x = " << x << std::endl;
```

executes as:

```cpp
((std::cout << "x = ") << x) << std::endl;
```

step by step:

```
std::cout << "x = "     // returns std::cout
          << x          // returns std::cout  
          << std::endl; // returns std::cout
```

## stream types

`std::cout`: standard output (terminal)
`std::cerr`: standard error
`std::ofstream`: file output stream

```cpp
std::ofstream file("output.txt");
file << "data";  // writes to file
```

## std::endl

special manipulator:

```cpp
std::cout << "line" << std::endl;
```

does two things:

. inserts newline character `\n`
. flushes the output buffer (forces immediate display)

equivalent to:

```cpp
std::cout << "line\n";
std::cout.flush();
```

## operator overloading

`<<` is the bitwise left shift operator, overloaded for streams.

original meaning:

```cpp
int x = 1 << 3;  // bitwise: shift 1 left by 3 positions = 8
```

overloaded for streams:

```cpp
std::cout << 42;  // stream insertion
```

same symbol, different behaviour based on types.

## why this syntax

allows natural left-to-right reading:

```cpp
std::cout << "value: " << x;
```

reads as: "send to cout: 'value: ', then x"

## key insight

`<<` sends data to output stream and returns stream, 
enabling chaining multiple insertions in natural left-to-right order.