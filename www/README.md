# www/

default document root for webserv.

this directory contains the static files the server delivers to the browser
when running with the default configuration. 
it is committed to the repository so the server is self-contained 
and runnable on any machine without external setup.


## structure

```
www/
    html/
        index.html          default landing page (served at /)
        errors/
            404.html        not found
            500.html        internal server error
```


## path portability

config files reference this directory via relative paths:

```
root ./www/html;
error_page 404 /errors/404.html;
```

at runtime, the server resolves relative paths against the process working
directory using getcwd(). this means the binary must always be launched
from the repository root:

```
./webserv [config]       correct
cd build && ./webserv    incorrect — path resolution will fail
```

absolute paths (e.g. root /var/www/html) would break on any machine
where that path does not exist — including the evaluator's machine.
relative paths resolve correctly anywhere the repo is cloned.


## precedent

shipping a default document root alongside the binary is standard practice
in production web servers:

. nginx:   ships with html/ containing index.html and 50x.html
  https://nginx.org/en/docs/beginners_guide.html

. apache:  ships with htdocs/ containing index.html
  https://httpd.apache.org/docs/2.4/getting-started.html

. gnu libmicrohttpd: minimal embedded server, leaves document root
  to the application — no bundled www/ directory
  https://www.gnu.org/software/libmicrohttpd/

the pattern exists precisely because portability demands it: a server
that requires manual filesystem setup before it can run is not portable.