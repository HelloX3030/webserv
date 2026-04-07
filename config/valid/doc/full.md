# full.conf

Integrated showcase for the webserv implementation.

## covered features

- static site with custom error pages
- redirect (`/old` -> `/blog/`)
- uploads (`/upload/<filename>` POST/GET/DELETE)
- cookies via CGI (`Set-Cookie` + readback)
- CGI (Python + Bash)
- virtual hosts on one port (`alpha.localhost` / `beta.localhost`)

## run

- `./webserv config/valid/full.conf`

## quick checks

- landing page: `http://127.0.0.1:8080/`
- redirect: `http://127.0.0.1:8080/old`
- blog: `http://127.0.0.1:8080/blog/`
- python cgi: `http://127.0.0.1:8080/cgi-python/env.py`
- bash cgi: `http://127.0.0.1:8080/cgi-bash/env.sh`
- cookies: `http://127.0.0.1:8080/cookies/`
- uploads: `http://127.0.0.1:8080/uploads/`

## host routing

- `curl -H "Host: alpha.localhost" http://127.0.0.1:8080/`
- `curl -H "Host: beta.localhost"  http://127.0.0.1:8080/`
