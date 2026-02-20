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
        index.html          default landing page — served at GET /
        upload.html         file upload form — served at GET / under uploads.conf
        errors/
            404.html        not found
            500.html        internal server error
    uploads/
        README.md           tracks the directory in git; uploaded files excluded
```


## file roles

**html/index.html**
the evaluator's first impression when running default.conf. GET / resolves
here via the index directive.

**html/upload.html**
an HTML form that POSTs to /upload/ when running uploads.conf. the critical
attribute is `enctype="multipart/form-data"` — this encoding transmits binary
file data intact and includes the Content-Disposition header carrying the
filename. without it the browser sends application/x-www-form-urlencoded,
which cannot carry binary data and transmits no filename. the server extracts
the filename from Content-Disposition, sanitises it via basename() to strip
directory components, and writes the received bytes to upload_store under
that flat filename.

**html/errors/404.html, 500.html**
served when the server's error_page directive maps a status code to these
paths. paths are URI-space, not filesystem paths — the server resolves them
against the location root at request time.

**uploads/**
runtime write target for upload_store. the directory must exist before the
server starts — a missing upload_store is a configuration error caught at
startup. uploaded files are excluded from version control via .gitignore;
the directory is tracked solely via its README.md.


## serving vs. executing

the server returns the bytes of files in this tree to the client. it does
not execute them. the boundary matters:

. a file served → client receives raw bytes. the client (browser) decides
  what to do with them based on Content-Type.
. a file executed → the server runs its content as code and returns the output.
  this is CGI, and it is scoped exclusively to locations with cgi_extension
  and cgi_path configured — never to www/html/ or www/uploads/.

the separation of upload_store from cgi_path is architectural, not incidental.
it is what prevents the canonical unrestricted file upload vulnerability
(CWE-434): a client uploads a .php file, then GETs it, and a misconfigured
server executes it. webserv avoids this structurally — no extension in
www/uploads/ triggers execution.

reference: OWASP — unrestricted file upload
https://owasp.org/www-community/vulnerabilities/Unrestricted_File_Upload
CWE-434: https://cwe.mitre.org/data/definitions/434.html


## path portability

config files reference this directory via relative paths:

```
root ./www/html;
error_page 404 /errors/404.html;
upload_store ./www/uploads;
```

at runtime the server resolves relative paths against the process working
directory using getcwd(). the binary must therefore always be launched from
the repository root:

```
./webserv [config]       correct
cd build && ./webserv    incorrect — path resolution will fail
```

absolute paths (e.g. root /var/www/html) would break on any machine where
that path does not exist — including the evaluator's machine. relative paths
resolve correctly anywhere the repo is cloned.


## precedent

shipping a default document root alongside the binary is standard practice
in production web servers:

. nginx:   ships with html/ containing index.html and 50x.html
  https://nginx.org/en/docs/beginners_guide.html

. apache:  ships with htdocs/ containing index.html
  https://httpd.apache.org/docs/2.4/getting-started.html

. gnu libmicrohttpd: minimal embedded server, leaves document root to the
  application — no bundled www/ directory
  https://www.gnu.org/software/libmicrohttpd/

the pattern exists precisely because portability demands it: a server that
requires manual filesystem setup before it can run is not portable.