# config/

configuration files for webserv.


## test suite philosophy

one file per use-case/scenario. each file demonstrates a coherent
operational scenario the evaluator might test, not an isolated directive.
this keeps the set small, readable, and directly mapped to eval criteria.


## files

### basic.conf

parser development artifact. the smallest config that satisfies every
mandatory validator constraint and nothing more. not a real runnable
scenario — used to verify the parser accepts valid minimal input and
rejects nothing it shouldn't.

mandatory constraints exercised:
. at least one listen address per server
. at least one location per server
. root present in every location

path dependencies: none (not intended to be run).

---

### default.conf

loaded when the binary is invoked with no argument:

```
./webserv
```

must be a real, runnable config. points to paths that exist in the repo
(www/). this is the evaluator's first impression of the server.

demonstrates: listen, server_name, root, index, error_page,
client_max_body_size, allowed_methods.

path dependencies: www/html/, www/html/errors/

---

### multi-server.conf

virtual hosting: two server configurations sharing one port, each
identified by a different server_name. the runtime selects which
configuration handles a request by comparing the HTTP Host header
against each server's server_name list.

a server configuration here means one server { } unit in the config
file and the corresponding ServerConfig object in memory.

demonstrates: multiple server configurations on one port, server_name
routing, the autoindex directive (one server uses it, one does not).

path dependencies: www/html/ (reused across both servers for simplicity)

fallback (decided): if the Host header matches no server_name, the first
server configuration defined on that port handles the request. this is
nginx behaviour and our adopted convention. runtime must iterate
ServerConfig objects in parse order and return the first match on
host:port when no server_name matches. config parser unaffected —
it already stores configs in parse order.

---

### cgi.conf

dynamic content via CGI. the server forks a subprocess, pipes the
request to it, and returns its stdout as the response.

demonstrates: cgi_extension, cgi_path, location scoped to /cgi-bin/.

path dependencies: www/cgi-bin/ (cgi scripts live here)

---

### uploads.conf

file upload scenario. client POSTs a file; server saves it to disk.

demonstrates: upload_enable, upload_store, POST in allowed_methods,
client_max_body_size set high enough to accept uploads.

path dependencies: www/uploads/ (server writes uploaded files here)

---

### full.conf

all directives combined in one config. the comprehensive scenario:
static files, error pages, method restrictions, CGI, uploads, body
size limits, autoindex, virtual hosting, redirects.

intended as a final integration test and eval demonstration.

path dependencies: all of the above.


## directives reference

all directive names are final.

### server-level

```
listen               <port> | <host>:<port>
server_name          <n> [<n> ...]
client_max_body_size <size>[k|m|g]
error_page           <status_code> <path>
```

### location-level

```
root                 <path>
index                <filename> [<filename> ...]
allowed_methods      GET | POST | DELETE
autoindex            on | off
cgi_extension        <.ext>
cgi_path             <path>
client_max_body_size <size>[k|m|g]
upload_enable        on | off
upload_store         <path>
return               <status_code> <path>
```

return adopted from nginx convention.
upload_enable and upload_store are webserv-specific.


## path resolution

relative paths in root and upload_store directives are resolved
at parse time via getcwd(). Location::root always contains an
absolute path after parsing. the binary must therefore always be
launched from the repository root:

```
./webserv [config]       correct
cd build && ./webserv    incorrect
```