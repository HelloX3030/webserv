## default.conf

loaded when the binary is invoked with no argument:

```
./webserv
```

must be a real, runnable config. points to paths that exist in the repo
(www/). 

demonstrates: listen, server_name, root, index, error_page,
client_max_body_size, allowed_methods.

path dependencies: www/html/, www/html/errors/