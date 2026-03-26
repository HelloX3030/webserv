# the explicit specifier


## essence

`explicit` prevents the compiler from using a constructor
for implicit type conversions.

without `explicit`: the compiler may silently call your constructor
to convert 1 type to another.

with `explicit`: the programmer must write the constructor call explicitly.


---


## the problem explicit solves

C++ performs implicit conversions.

given:

    ```cpp
    struct Seconds
    {
        Seconds(int n) : value(n) {}
        int value;
    };

    void wait(Seconds duration);
    ```

you can write:

    ```cpp
    wait(5);  // compiles: implicit conversion int → Seconds
    ```


the compiler sees: `wait` expects `Seconds`, you gave `int`.
it finds `Seconds(int)` and calls it silently.
`wait(5)` becomes `wait(Seconds(5))`.

this is sometimes convenient, but can be dangerous.


---


## when implicit conversion is dangerous

    ```cpp
    struct Filepath
    {
        Filepath(const char* path) : path_(path) {}
        std::string path_;
    };

    void delete_file(Filepath f);
    ```

now this compiles:

    ```cpp
    delete_file("important_data.txt");  // implicit conversion
    ```


perhaps intended. but also:

    ```cpp
    std::string name = get_user_input();
    delete_file(name.c_str());  // compiles silently
    ```

the programmer may not realise they're constructing a Filepath.
the implicit conversion hides intent.


worse — overload resolution becomes unpredictable:

    ```cpp
    void process(Filepath f);
    void process(std::string s);

    process("hello");  // which one? depends on conversion ranking
    ```

implicit conversions create action at a distance.
the call site doesn't show what's happening.


---


## explicit: the solution

    ```cpp
    struct Seconds
    {
        explicit Seconds(int n) : value(n) {}
        int value;
    };

    void wait(Seconds duration);
    ```

now:

    ```cpp
    wait(5);              // ERROR: no implicit conversion
    wait(Seconds(5));     // OK: explicit construction
    wait(Seconds{5});     // OK: explicit construction (brace syntax)
    ```

the programmer must write what they mean.
the call site reveals intent.


---


## when to use explicit

**default stance: use explicit on single-argument constructors.**

omit explicit only when implicit conversion is genuinely desirable —
when the types are semantically equivalent and conversion is always safe.

examples where implicit is acceptable:
- `std::string(const char*)` — a string literal *is* conceptually a string
- `std::string_view(const char*)` — same reasoning
- wrapper types that are transparent aliases

examples where explicit is required:
- `vector<int>(size_t n)` — an integer is not a vector
- `Seconds(int n)` — an integer is not a duration
- `HttpRequestFrontend(size_t max_body_size)` — a size is not a parser
- any constructor where the argument is configuration, not content


---


## explicit and multi-argument constructors

constructors with 2+ arguments cannot be called implicitly anyway —
there's no syntax for it. so `explicit` is redundant there.

exception: default arguments. if a 2-argument constructor has defaults
such that it can be called with 1 argument, `explicit` matters:

    ```cpp
    struct Foo
    {
        Foo(int x, int y = 0);  // can be called as Foo(5)
    };

    void take(Foo f);
    take(5);  // compiles: implicit conversion via Foo(5, 0)
    ```


add `explicit`:

    ```cpp
    struct Foo
    {
        explicit Foo(int x, int y = 0);
    };

    take(5);  // ERROR
    ```


---


## explicit in C++11 and beyond

C++11 extended `explicit` to conversion operators:

```cpp
struct Foo
{
    explicit operator bool() const;  // explicit conversion to bool
};

Foo f;
if (f) { }       // OK: contextual conversion to bool allowed
bool b = f;      // ERROR: implicit conversion disallowed
bool b = bool(f); // OK: explicit conversion
```

the `if` context is special — it allows explicit bool conversions.
this is why `std::optional`, `std::unique_ptr`, etc. use `explicit operator bool()`.
you can write `if (opt)` but not `bool b = opt;`.


C++20 added conditional explicit:

```cpp
template<typename T>
struct Wrapper
{
    template<typename U>
    explicit(!std::is_convertible_v<U, T>)
    Wrapper(U&& u);
};
```

the constructor is explicit iff U is not implicitly convertible to T.
this allows perfect forwarding of implicit/explicit semantics.


---


## the principle

explicit is about **preserving intent at the call site**.

implicit conversions hide what's happening.
explicit constructors force the programmer to write what they mean.

the cost: slightly more typing.
the benefit: code says what it does.

in systems programming, clarity beats brevity.
default to explicit.


---


## references

cppreference: explicit specifier
    https://en.cppreference.com/w/cpp/language/explicit.html

C++ Core Guidelines C.46: "By default, declare single-argument
    constructors explicit"
    https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c46-by-default-declare-single-argument-constructors-explicit
