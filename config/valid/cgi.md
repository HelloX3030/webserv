## cgi.conf

dynamic content via CGI. the server forks a subprocess, pipes the
request to it, and returns its stdout as the response.

demonstrates: cgi_extension, cgi_path, location scoped to /cgi-bin/.

path dependencies: www/cgi-bin/ (cgi scripts live here)