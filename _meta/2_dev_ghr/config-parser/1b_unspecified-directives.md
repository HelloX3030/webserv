**Where do the unspecified directives sit?**

Let me trace the full path of one directive from config file to C++ struct, 
using `root` as the example because it's fully specified.


The config file contains text:
```
root /var/www/html;

The grammar says:

```ebn
froot_dir = "root", path, ";" ;
```

This is the rule. It says: the word root, then a path, then a semicolon. 
That sequence of tokens means something — it means "set the root directory to this path."

The parser reads that token sequence and executes:
```cpp
location.root = "/var/www/html";
```

The data model defines where it lands:
```cpp
struct Location {
    std::string root;   // ← here
    ...
};
```

So the full chain for `root`:
```
text in file     →    grammar rule    →    parser action    →    field in struct
"root /var/www;" →    root_dir        →    location.root =  →    Location::root
```

---

Now the unspecified directives:
`return` and `upload_enable`/`upload_store` need the same chain — 
but steps 1 and 2 are not yet agreed:
```
text in file     →    grammar rule    →    parser action    →    field in struct
???              →    ???             →    ???              →    Location::return_code
                                                                 Location::return_path
                                                                 Location::upload_enable
                                                                 Location::upload_store

The fields exist in 2_data-model.md as commented stubs. 
The grammar rule exists as a proposal in 1_grammar.md. 
What is missing is the team decision on the directive names — 
and until that is made, you cannot write the parser case for them.

That is the only thing blocking them. They are not architecturally complex. 
Once the names are agreed, each one is two lines in the parser and one field in the struct. 
The note exists precisely to mark that gap so it is not forgotten.
