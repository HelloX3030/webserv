# www/uploads/

runtime upload target for webserv.

the server writes files received via POST to this directory
when a location block is configured with:

    upload_enable on;
    upload_store ./www/uploads;

## properties

. this directory is managed by the server process at runtime — do not
  commit uploaded files to the repository.
. the directory itself must be tracked (hence this file) so that
  `git clone` + `./webserv config/uploads.conf` works without manual setup.
. the server does not create this directory if it is missing —
  a missing upload_store is a configuration error caught at startup.

## filename handling

filenames are extracted from the `Content-Disposition: filename=` parameter
of the multipart request header and sanitised via basename() before writing.
all directory components are stripped. path traversal attempts
(e.g. `../../etc/passwd`) are reduced to a flat filename.

## gitignore

uploaded files are excluded from version control via .gitignore:

    www/uploads/*
    !www/uploads/README.md