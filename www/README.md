# www/

the document root. the filesystem subtree the server maps to URI-space
and delivers to HTTP clients. when the server receives a GET request,
it resolves the URI against this tree, reads the file, and transmits
its bytes.

committed to the repository so the server is self-contained:
`git clone` + `./webserv` works on any machine without external setup.


## naming

`www/` is the convention of early web servers (Apache, NCSA httpd).
alternatives: `htdocs/`, `public_html/`, `html/`.
the name carries no runtime meaning — what the server treats as its
document root is determined solely by the `root` directive in config.


## structure

see local disk filesystem. this directory is a work in progress.


## files

### html/index.html

the landing page. resolved by the `index` directive when a GET request
targets a directory (e.g. GET /). the evaluator's first contact with
a running server under default.conf.


### html/upload.html

a web page containing a file upload form. the server delivers it to the
browser in response to GET /upload (under uploads.conf). the browser
renders it: the user sees a file picker and a submit button.

when the user submits, the browser constructs a POST request. the form's
`enctype="multipart/form-data"` attribute instructs the browser to use
multipart encoding — the only encoding that can carry binary file data.
under this encoding, the request body is divided into MIME parts: one
part carries the file bytes, another carries metadata including
`Content-Disposition: filename=`, from which the server extracts the
original filename.

without `enctype="multipart/form-data"`, the browser falls back to
`application/x-www-form-urlencoded`, which cannot carry binary data
and transmits no filename. the upload silently fails.


### errors/

error pages served when the server's `error_page` directive maps a
status code to a path in this directory. this directory is a work
in progress.

paths in `error_page` are URI-space paths, not filesystem paths:
the server resolves them against the location root at request time.


### uploads/

runtime write target. the server writes files received via POST here
when a location block is configured with:

    upload_enable on;
    upload_store ./www/uploads;

the directory must exist before the server starts — a missing
`upload_store` is a configuration error caught at startup.

uploaded files are excluded from version control. the directory is
tracked solely via its README.md, so `git clone` leaves it present.


## serving vs. executing

the server transmits the bytes of files in this tree to the client.
it does not execute them. the distinction matters:

. served: client receives raw bytes. the client (browser) interprets
  them according to Content-Type.
. executed: the server runs the file's content as code and returns
  the output. this is CGI — scoped exclusively to locations with
  `cgi_extension` and `cgi_path` configured, never to www/.


## security

the separation of `upload_store` from `cgi_path` is architectural,
not incidental. it is what prevents the canonical unrestricted file
upload vulnerability (CWE-434): a client uploads a .php file, GETs it,
and a misconfigured server executes it. webserv avoids this structurally
— no path under www/uploads/ triggers execution.

reference: OWASP — unrestricted file upload
https://owasp.org/www-community/vulnerabilities/Unrestricted_File_Upload
CWE-434: https://cwe.mitre.org/data/definitions/434.html


## path portability

config files reference this directory via relative paths:

    root ./www/html;
    error_page 404 /errors/404.html;
    upload_store ./www/uploads;

relative paths are resolved against the process working directory.
the binary must be launched from the repository root:

    ./webserv [config]       correct
    cd build && ./webserv    incorrect — path resolution fails