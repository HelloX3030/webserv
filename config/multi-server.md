## multi-server.conf

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